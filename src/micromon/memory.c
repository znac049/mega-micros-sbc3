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

/*
 * The SBC-3 has two RAM areas, each an 8MB window:
 *
 *   $00000000-$007FFFFF   2MB, 4MB or 8MB fitted
 *   $01000000-$017FFFFF   nothing, 2MB or 8MB fitted
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

static inline void write_cacr(uint32_t cacr) {
    __asm__ volatile ("movec %0,%%cacr" : : "d" (cacr) : "memory");
}

/*
 * Size the RAM area starting at base ($00000000 or $01000000). Returns the
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
        write_cacr((cacr & ~CACR_ED) | CACR_CD);
    }

    size = probe_area(base);

    if (cacr & CACR_ED) {
        write_cacr(cacr | CACR_CD);     // back on, without stale lines
    }

    restore_sr(sr);

    return size;
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

int test_ram(uint32_t start_addr, uint32_t end_addr) {
    int i = 0;
    uint8_t aborted = NO;

    if (start_addr & 3)
        start_addr++;
    
    if (end_addr && 3) 
        end_addr--;

    printk(" Testing......");
    for (uint32_t *addr=(uint32_t *)start_addr; addr<(uint32_t *)end_addr; addr++) {
        uint32_t orig = addr[0];
        uint32_t x,y;

        addr[0] = 0x55555555;
        x = addr[0];

        addr[0] = 0xaaaaaaaa;
        y = addr[0];
        
        addr[0] = orig;

        if (!((x == 0x55555555) && (y == 0xaaaaaaaa))) {
            printk("\nMemory test failed at $%08x\n", addr);
            return (int)addr;
        }

        if (++i == 0x00020000) {
            i = 0;
            spin();

            if (kchar_available()) {
                addr = (uint32_t *)end_addr;
                aborted = YES;
                kgetchar();
            }
        }
    }

    for (int j=0; j<6; j++) {
        printk("%c", BS);
    }

    if (aborted == YES) {
        printk(": ABORTED       \n");
    }
    else {
        printk(": PASS          \n");
    }

    return OK;
}