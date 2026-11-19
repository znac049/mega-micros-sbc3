/* 
MIT License

Copyright (c) 2026 Bob Green

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <stddef.h>
#include <ctype.h>
#include <machine.h>

#include "micromon.h"
#include "memory.h"

// From the linker script: the top of the monitor's stack
extern char _INITIAL_STACK[];

/*
 * The SBC-3 has two RAM areas, each an 8MB window:
 *
 *   $00000000-$007FFFFF   2MB, 4MB or 8MB fitted
 *   $10000000-$107FFFFF   nothing, 2MB or 8MB fitted
 *
 * The CPLD generates DTACK for the whole of a populated 8MB window, however
 * much memory is fitted, so running off the end of the RAM doesn't cause a
 * bus error - the memory just repeats (2MB fitted appears 4 times, 4MB
 * twice). So we size an area by looking for that wrap-around: write one
 * marker at a test location, another 2MB (then 4MB) above it, and see
 * whether the second write landed on top of the first.
 *
 * Every location touched is saved first and put back afterwards, with
 * interrupts masked, so this is safe to run while the monitor is using
 * the memory (including the vectors, data and stack in the low area).
 * Accesses go through the bus-error safe peekl()/pokel(), so an
 * unpopulated window simply reports zero.
 */


static inline uint16_t check_address(volatile uint16_t *addr) {
    uint16_t orig = addr[0];
    uint16_t x,y;

    addr[0] = 0x5555;
    x = addr[0];

    addr[0] = 0xaaaa;
    y = addr[0];
    
    addr[0] = orig;

    return ((x == 0x5555) && (y == 0xaaaa));
}

static void spin(void) {
    static char flips[4] = "|/-\\";
    static int i=0;

    printk("%c%c", BS, flips[i++]);
    if (i == sizeof(flips)) {
        i = 0;
    }
}

static uint32_t rd(volatile uint32_t *addr, int *bad) {
    int err;
    uint32_t val = peekl(addr, &err);

    if (err) {
        *bad = YES;
    }

    return val;
}

static void wr(volatile uint32_t *addr, uint32_t val, int *bad) {
    int err;

    pokel(addr, val, &err);

    if (err) {
        *bad = YES;
    }
}

/*
 * Size one area. Returns the number of bytes fitted, or 0 if there's
 * no memory there at all.
 */
static uint32_t probe_area(uint32_t base) {
    static const uint32_t candidates[] = { TWO_MEG, FOUR_MEG };
    volatile uint32_t *cell = (volatile uint32_t *)(base + RAM_PROBE_OFFSET);
    volatile uint32_t *next = cell + 1;
    uint32_t save_cell, save_next;
    uint32_t size = RAM_AREA_MAX;
    int bad = NO;

    save_cell = rd(cell, &bad);
    save_next = rd(next, &bad);
    if (bad) {
        return 0;                       // bus error: nothing fitted
    }

    /*
     * Is there memory here at all? Use two cells, so a floating data bus
     * that remembers the last value written can't pass for memory.
     */
    wr(cell, MARK_BASE, &bad);
    wr(next, ~MARK_BASE, &bad);

    if ((rd(cell, &bad) != MARK_BASE) || (rd(next, &bad) != ~MARK_BASE) || bad) {
        wr(next, save_next, &bad);
        wr(cell, save_cell, &bad);

        return 0;
    }

    wr(next, save_next, &bad);

    // Look for the memory repeating at 2MB, then 4MB
    for (unsigned i=0; i<sizeof(candidates)/sizeof(candidates[0]); i++) {
        volatile uint32_t *alias = (volatile uint32_t *)((uint32_t)cell + candidates[i]);
        uint32_t save_alias = rd(alias, &bad);
        uint32_t base_val, alias_val;

        wr(cell, MARK_BASE, &bad);
        wr(alias, MARK_ALIAS, &bad);

        base_val = rd(cell, &bad);
        alias_val = rd(alias, &bad);

        /*
         * Put the alias back first: if it's really the same location as
         * cell, the restore of cell below has the last word.
         */
        wr(alias, save_alias, &bad);
        wr(cell, save_cell, &bad);

        /*
         * Either the write above landed on cell (the memory wraps here),
         * or the location above didn't hold what we wrote (no memory
         * there). Either way, this is where the memory ends.
         */
        if (bad || (base_val != MARK_BASE) || (alias_val != MARK_ALIAS)) {
            size = candidates[i];
            break;
        }
    }

    return size;
}

static inline uint16_t mask_irqs(void) {
    uint16_t sr;

    __asm__ volatile ("move.w %%sr,%0\n\tor.w #0x0700,%%sr" : "=d" (sr) : : "memory", "cc");

    return sr;
}

static inline void restore_sr(uint16_t sr) {
    __asm__ volatile ("move.w %0,%%sr" : : "d" (sr) : "memory", "cc");
}

/*
 * Size the RAM area starting at base ($00000000 or $10000000). Returns the
 * number of bytes fitted (2MB, 4MB or 8MB), or 0 if none.
 */
uint32_t size_ram_area(uint32_t base) {
    uint16_t sr = mask_irqs();
    uint32_t cacr = get_cacr();
    uint32_t size;

    /*
     * With the data cache on, reading the test location could hit the
     * cache instead of the memory, hiding the wrap-around. Turn it off
     * while we look (only touching the CACR if it's on).
     */
    if (cacr & CACR_ED) {
        set_cacr((cacr & ~CACR_ED) | CACR_CD);
    }

    size = probe_area(base);

    if (cacr & CACR_ED) {
        set_cacr(cacr | CACR_CD);     // back on, without stale lines
    }

    restore_sr(sr);

    return size;
}

/*
 * Is the area at base just the low area seen again? That happens when the
 * glue logic ignores the upper address lines: the original CPLD, or ours
 * built with 2MB banks. Write a marker through base and see if it turns
 * up in the low area.
 */
static int is_alias_of_low(uint32_t base) {
    volatile uint32_t *low = (volatile uint32_t *)(RAM_LOW_BASE + RAM_PROBE_OFFSET);
    volatile uint32_t *high = (volatile uint32_t *)(base + RAM_PROBE_OFFSET);
    uint16_t sr = mask_irqs();
    uint32_t cacr = get_cacr();
    uint32_t save_low, save_high, seen;
    int bad = NO;

    if (cacr & CACR_ED) {
        set_cacr((cacr & ~CACR_ED) | CACR_CD);
    }

    save_low = rd(low, &bad);
    save_high = rd(high, &bad);

    wr(low, MARK_BASE, &bad);
    wr(high, MARK_ALIAS, &bad);
    seen = rd(low, &bad);

    // High first: if they're the same cell, the low restore wins
    wr(high, save_high, &bad);
    wr(low, save_low, &bad);

    if (cacr & CACR_ED) {
        set_cacr(cacr | CACR_CD);
    }

    restore_sr(sr);

    return (bad || (seen == MARK_ALIAS)) ? YES : NO;
}

/*
 * Size both RAM areas. Either pointer may be NULL.
 */
void size_ram(uint32_t *low_size, uint32_t *high_size) {
    if (low_size != NULL) {
        *low_size = size_ram_area(RAM_LOW_BASE);
    }

    if (high_size != NULL) {
        *high_size = size_ram_area(RAM_HIGH_BASE);

        // Don't count bank 0 twice if the high area is just a repeat of it
        if ((*high_size != 0) && is_alias_of_low(RAM_HIGH_BASE)) {
            *high_size = 0;
        }
    }
}

/*
 * Last address of the low RAM area.
 */
uint32_t get_ram_end(void) {
    uint32_t size = size_ram_area(RAM_LOW_BASE);

    // We're running in it, so it can't really be missing
    if (size == 0) {
        size = FOUR_MEG;
    }

    return RAM_LOW_BASE + size -1;
}

/*
 * RAM tests
 *
 * These are destructive: they're only run on memory nothing is using.
 * At boot that's everything above the monitor's stack (see
 * ram_test_start()); the bottom of bank 0 is checked by ram_test in
 * ram_test.asm before any C code runs.
 *
 * mem_test_quick() runs, in order:
 *
 *   1  Data bus      walking 1s and 0s at one longword. A guard value is
 *                    written next door between the write and the read, so
 *                    a floating bus can't hand back the last value written.
 *   2  Byte lanes    byte and word writes, read back as a longword. These
 *                    use the CPLD's per-byte write strobes, which longword
 *                    writes never exercise.
 *   3  Address bus   a marker at start and at start + 4, 8, 16 ... Writing
 *                    each one in turn must leave all the others alone, so a
 *                    stuck, open or shorted address line shows up here,
 *                    with the faulty offset in the report.
 *   4  Own address   every longword holds its own address: the whole
 *                    range is written before anything is read back, so
 *                    any two addresses that reach the same cell (a decode
 *                    fault) or a stuck bit shows up.
 *   5  Complement    the same with the complement, so every bit of every
 *                    cell has held both a 0 and a 1.
 *
 * mem_test_march() adds a March C- test (stage 6), repeated over several
 * data backgrounds, for soak testing with the memtest command:
 *
 *   up(w0) up(r0,w1) up(r1,w0) down(r0,w1) down(r1,w0) up(r0)
 *
 * which finds stuck-at, transition and most coupling faults between
 * cells, and the backgrounds find coupling between bits of one longword.
 *
 * The data cache is turned off while testing (it would answer the reads
 * instead of the RAM). Interrupts stay on: the handlers only touch the
 * monitor's own variables and stack, which are never in a tested range.
 * Any key aborts.
 */

#define TEST_PATTERN    0xaaaaaaaaU
#define TEST_ANTI       0x55555555U
#define PROGRESS_LONGS  0x20000         // spin and check for a key this often

static const char *stage_names[] = {
    "", "data bus", "byte lanes", "address bus", "own address",
    "complement", "march C-"
};

static const uint32_t backgrounds[] = {
    0x00000000, 0x55555555, 0x33333333, 0x0f0f0f0f, 0x00ff00ff, 0x0000ffff
};

static uint32_t progress_count;
static bool_t show_progress;
static bool_t test_aborted;

/*
 * Called once per longword in the long passes. Returns YES if a key has
 * been pressed, which aborts the test.
 */
static inline bool_t progress(void) {
    if (++progress_count >= PROGRESS_LONGS) {
        progress_count = 0;

        if (show_progress) {
            spin();
        }

        if (kchar_available()) {
            kgetchar();
            test_aborted = YES;
        }
    }

    return test_aborted;
}

static int fail(memtest_result_t *r, int stage, volatile uint32_t *addr, uint32_t expected, uint32_t actual) {
    int err;

    r->stage = stage;
    r->addr = (uint32_t)addr;
    r->expected = expected;
    r->actual = actual;

    // Failing address on the hex display, if there is one
    pokel(hex_long_0, (uint32_t)addr, &err);

    return NOT_OK;
}

static uint32_t dcache_off(void) {
    uint32_t cacr = get_cacr();

    if (cacr & CACR_ED) {
        set_cacr((cacr & ~CACR_ED) | CACR_CD);
    }

    return cacr;
}

static void dcache_restore(uint32_t cacr) {
    if (cacr & CACR_ED) {
        set_cacr(cacr | CACR_CD);
    }
}

// 1: data bus
static int test_data_bus(volatile uint32_t *p, memtest_result_t *r) {
    for (uint32_t bit = 1; bit != 0; bit <<= 1) {
        uint32_t v;

        p[0] = bit;
        p[1] = ~bit;            // drive the bus the other way
        v = p[0];
        if (v != bit) {
            return fail(r, 1, p, bit, v);
        }

        p[0] = ~bit;
        p[1] = bit;
        v = p[0];
        if (v != ~bit) {
            return fail(r, 1, p, ~bit, v);
        }
    }

    return OK;
}

// 2: byte lanes
static int test_byte_lanes(volatile uint32_t *p, memtest_result_t *r) {
    volatile uint8_t *b = (volatile uint8_t *)p;
    volatile uint16_t *w = (volatile uint16_t *)p;
    uint32_t v;

    p[0] = 0;
    b[0] = 0x11;
    b[1] = 0x22;
    b[2] = 0x33;
    b[3] = 0x44;
    p[1] = 0xffffffff;
    if ((v = p[0]) != 0x11223344) {
        return fail(r, 2, p, 0x11223344, v);
    }

    b[2] = 0xcc;                // one byte in the middle
    p[1] = 0;
    if ((v = p[0]) != 0x1122cc44) {
        return fail(r, 2, p, 0x1122cc44, v);
    }

    w[1] = 0x5566;              // low word only
    p[1] = 0xffffffff;
    if ((v = p[0]) != 0x11225566) {
        return fail(r, 2, p, 0x11225566, v);
    }

    w[0] = 0x7788;              // high word only
    p[1] = 0;
    if ((v = p[0]) != 0x77885566) {
        return fail(r, 2, p, 0x77885566, v);
    }

    // And the bytes read back one at a time
    if ((b[0] != 0x77) || (b[1] != 0x88) || (b[2] != 0x55) || (b[3] != 0x66)) {
        return fail(r, 2, p, 0x77885566, ((uint32_t)b[0] << 24) | ((uint32_t)b[1] << 16) | ((uint32_t)b[2] << 8) | b[3]);
    }

    return OK;
}

// 3: address bus. size is in bytes, a multiple of 4.
static int test_address_bus(uint32_t start, uint32_t size, memtest_result_t *r) {
    volatile uint32_t *base = (volatile uint32_t *)start;
    uint32_t off, other, v;

    // A marker everywhere
    base[0] = TEST_PATTERN;
    for (off = 4; off < size; off <<= 1) {
        base[off / 4] = TEST_PATTERN;
    }

    // Lines stuck high: changing the base mustn't change the others
    base[0] = TEST_ANTI;
    for (off = 4; off < size; off <<= 1) {
        if ((v = base[off / 4]) != TEST_PATTERN) {
            return fail(r, 3, &base[off / 4], TEST_PATTERN, v);
        }
    }
    base[0] = TEST_PATTERN;

    // Lines stuck low or shorted together: change each in turn
    for (off = 4; off < size; off <<= 1) {
        base[off / 4] = TEST_ANTI;

        if ((v = base[0]) != TEST_PATTERN) {
            return fail(r, 3, &base[0], TEST_PATTERN, v);
        }

        for (other = 4; other < size; other <<= 1) {
            if ((other != off) && ((v = base[other / 4]) != TEST_PATTERN)) {
                return fail(r, 3, &base[other / 4], TEST_PATTERN, v);
            }
        }

        base[off / 4] = TEST_PATTERN;
    }

    return OK;
}

// 4 and 5: every longword holds its own address, xor'd with mask
static int test_fill(uint32_t start, uint32_t end, uint32_t mask, int stage, memtest_result_t *r) {
    volatile uint32_t *p;
    uint32_t v;

    for (p = (volatile uint32_t *)start; p < (volatile uint32_t *)end; p++) {
        *p = (uint32_t)p ^ mask;
        if (progress()) {
            return OK;
        }
    }

    for (p = (volatile uint32_t *)start; p < (volatile uint32_t *)end; p++) {
        if ((v = *p) != ((uint32_t)p ^ mask)) {
            return fail(r, stage, p, (uint32_t)p ^ mask, v);
        }
        if (progress()) {
            return OK;
        }
    }

    return OK;
}

// 6: March C-, with data background bg
static int test_march(uint32_t start, uint32_t end, uint32_t bg, memtest_result_t *r) {
    volatile uint32_t *lo = (volatile uint32_t *)start;
    volatile uint32_t *hi = (volatile uint32_t *)end;
    volatile uint32_t *p;
    const uint32_t d0 = bg;
    const uint32_t d1 = ~bg;
    uint32_t v;

#define MARCH_CHECK(want) \
    if ((v = *p) != (want)) { \
        return fail(r, 6, p, (want), v); \
    } \
    if (progress()) { \
        return OK; \
    }

    // up(w0)
    for (p = lo; p < hi; p++) {
        *p = d0;
        if (progress()) {
            return OK;
        }
    }

    // up(r0, w1)
    for (p = lo; p < hi; p++) {
        MARCH_CHECK(d0);
        *p = d1;
    }

    // up(r1, w0)
    for (p = lo; p < hi; p++) {
        MARCH_CHECK(d1);
        *p = d0;
    }

    // down(r0, w1)
    for (p = hi; p-- > lo; ) {
        MARCH_CHECK(d0);
        *p = d1;
    }

    // down(r1, w0)
    for (p = hi; p-- > lo; ) {
        MARCH_CHECK(d1);
        *p = d0;
    }

    // up(r0)
    for (p = lo; p < hi; p++) {
        MARCH_CHECK(d0);
    }

#undef MARCH_CHECK

    return OK;
}

/*
 * Stages 1-5 over start..end (inclusive). Returns OK if it passed or was
 * aborted (r->aborted says which), NOT_OK with the details in r if not.
 */
int mem_test_quick(uint32_t start, uint32_t end, bool_t verbose, memtest_result_t *r) {
    uint32_t cacr;
    int res;

    // Whole longwords, end exclusive from here on
    start = (start + 3) & ~3U;
    end = (end + 1) & ~3U;

    r->stage = 0;
    r->aborted = NO;

    if (end < start + 16) {
        return OK;              // too small to bother
    }

    show_progress = verbose;
    test_aborted = NO;
    progress_count = 0;
    cacr = dcache_off();

    res = test_data_bus((volatile uint32_t *)start, r);
    if (res == OK) {
        res = test_byte_lanes((volatile uint32_t *)start, r);
    }
    if (res == OK) {
        res = test_address_bus(start, end - start, r);
    }
    if ((res == OK) && !test_aborted) {
        res = test_fill(start, end, 0, 4, r);
    }
    if ((res == OK) && !test_aborted) {
        res = test_fill(start, end, 0xffffffff, 5, r);
    }

    dcache_restore(cacr);
    r->aborted = test_aborted;

    return res;
}

/*
 * Stage 6 (March C-) over start..end (inclusive), once per data
 * background. Prints the background being tested if verbose.
 */
int mem_test_march(uint32_t start, uint32_t end, bool_t verbose, memtest_result_t *r) {
    uint32_t cacr;
    int res = OK;

    start = (start + 3) & ~3U;
    end = (end + 1) & ~3U;

    r->stage = 0;
    r->aborted = NO;

    if (end <= start) {
        return OK;
    }

    show_progress = verbose;
    test_aborted = NO;
    progress_count = 0;
    cacr = dcache_off();

    for (unsigned i = 0; (i < sizeof(backgrounds) / sizeof(backgrounds[0])) && (res == OK) && !test_aborted; i++) {
        if (verbose) {
            printk(" $%08X ", backgrounds[i]);
        }

        res = test_march(start, end, backgrounds[i], r);

        if (verbose) {
            for (int j = 0; j < 11; j++) {
                printk("%c", BS);
            }
        }
    }

    dcache_restore(cacr);
    r->aborted = test_aborted;

    return res;
}

const char *mem_test_stage_name(int stage) {
    if ((stage < 0) || (stage >= (int)(sizeof(stage_names) / sizeof(stage_names[0])))) {
        return "?";
    }

    return stage_names[stage];
}

void pr_mem_test_failure(const memtest_result_t *r) {
    printk("\n    Failed: stage %d (%s) at $%08X: expected $%08X, read $%08X (bits $%08X)\n",
           r->stage, mem_test_stage_name(r->stage), r->addr,
           r->expected, r->actual, r->expected ^ r->actual);
}

/*
 * The lowest address the boot test and the memtest command may use: above
 * the monitor's stack and variables. Below this, bank 0 is checked by
 * ram_test.asm at reset.
 */
uint32_t ram_test_start(void) {
    uint32_t start = (uint32_t)_INITIAL_STACK + 4;

    if ((uint32_t)&_bss_end > start) {
        start = (uint32_t)&_bss_end;
    }

    return (start + 3) & ~3U;
}

/*
 * The boot-time test, with the spinner and the PASS/FAIL report that
 * follows each RAM line of the banner.
 */
int test_ram(uint32_t start_addr, uint32_t end_addr) {
    memtest_result_t r;
    int res;

    if (heap_range_in_use(start_addr, end_addr)) {
        printk(" In use: not tested\n");
        return OK;
    }

    printk(" Testing......");
    res = mem_test_quick(start_addr, end_addr, YES, &r);

    for (int j=0; j<6; j++) {
        printk("%c", BS);
    }

    if (res != OK) {
        printk(": FAIL          ");
        pr_mem_test_failure(&r);
    }
    else if (r.aborted == YES) {
        printk(": ABORTED       \n");
    }
    else {
        printk(": PASS          \n");
    }

    return res;
}
