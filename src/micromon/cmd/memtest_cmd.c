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
#include <machine.h>
#include <extras.h>

#include "micromon.h"
#include "memory.h"
#include "expr.h"

/*
 * memtest [<start> <end> [<passes>]]
 *
 * Soak-test RAM: each pass runs the quick test (data bus, byte lanes,
 * address bus, own-address fills) and then March C- over six data
 * backgrounds. With no range it tests all the RAM the monitor isn't
 * using. The test is destructive, so a range must be free: not the
 * monitor's own area, and not anything allocated from the heap. Programs
 * loaded with 'load' aren't known to the heap, so don't test over one
 * you still want. Any key stops it.
 */

typedef struct {
    uint32_t start;
    uint32_t end;               // inclusive
} mt_range_t;

static bool_t in_bank(uint32_t start, uint32_t end) {
    // Bank 0 starts at 0 (RAM_LOW_BASE)
    if (end < RAM_LOW_BASE + ram_low_size) {
        return YES;
    }

    if ((ram_high_size != 0) && (start >= RAM_HIGH_BASE) && (end < RAM_HIGH_BASE + ram_high_size)) {
        return YES;
    }

    return NO;
}

static int parse_arg(const char *arg, long *val) {
    int error_pos;

    if (expr_evaluate(arg, val, &error_pos) != EXPR_OK) {
        printk("Couldn't evaluate expression: '%s'\n", arg);
        return NOT_OK;
    }

    return OK;
}

/*
 * One pass over one range. Returns OK, or NOT_OK on a failure or abort.
 */
static int test_range(const mt_range_t *range) {
    memtest_result_t r;

    printk("  $%08X-$%08X: quick......", range->start, range->end);
    if (mem_test_quick(range->start, range->end, YES, &r) != OK) {
        pr_mem_test_failure(&r);
        return NOT_OK;
    }
    if (r.aborted) {
        printk("\n");
        return NOT_OK;
    }
    printk("%c ok, march C-......", BS);

    if (mem_test_march(range->start, range->end, YES, &r) != OK) {
        pr_mem_test_failure(&r);
        return NOT_OK;
    }
    if (r.aborted) {
        printk("\n");
        return NOT_OK;
    }
    printk("%c ok\n", BS);

    return OK;
}

void handle_memtest_command(int argc, char *argv[]) {
    mt_range_t ranges[2];
    int num_ranges = 0;
    long passes = 1;
    long val;

    if ((argc == 2) || (argc > 4)) {
        printk("Usage: memtest [<start> <end> [<passes>]]\n");
        return;
    }

    if (argc == 1) {
        // Everything that's free
        mt_range_t low = { ram_test_start(), RAM_LOW_BASE + ram_low_size - 1 };

        if (heap_range_in_use(low.start, low.end)) {
            printk("Bank 0 above $%08X is in use; give a range to test part of it\n", low.start);
        }
        else {
            ranges[num_ranges++] = low;
        }

        if (ram_high_size != 0) {
            mt_range_t high = { RAM_HIGH_BASE, RAM_HIGH_BASE + ram_high_size - 1 };
            ranges[num_ranges++] = high;
        }
    }
    else {
        mt_range_t range;

        if (parse_arg(argv[1], &val) != OK) {
            return;
        }
        range.start = (uint32_t)val;

        if (parse_arg(argv[2], &val) != OK) {
            return;
        }
        range.end = (uint32_t)val;

        if (argc == 4) {
            if (parse_arg(argv[3], &passes) != OK) {
                return;
            }
        }

        if (range.end < range.start + 15) {
            printk("The range must be at least 16 bytes\n");
            return;
        }

        if (!in_bank(range.start, range.end)) {
            printk("$%08X-$%08X isn't all in one bank of RAM\n", range.start, range.end);
            return;
        }

        if (range.start < ram_test_start()) {
            printk("The monitor uses RAM below $%08X: start at or above that\n", ram_test_start());
            return;
        }

        if (heap_range_in_use(range.start, range.end)) {
            printk("$%08X-$%08X is in use\n", range.start, range.end);
            return;
        }

        ranges[num_ranges++] = range;
    }

    if (num_ranges == 0) {
        return;
    }

    if (passes < 1) {
        passes = 1;
    }

    printk("Testing (any key stops)\n");

    for (long pass = 1; pass <= passes; pass++) {
        printk("Pass %d of %d\n", pass, passes);

        for (int i = 0; i < num_ranges; i++) {
            if (test_range(&ranges[i]) != OK) {
                return;
            }
        }
    }

    printk("All passes OK\n");
}
