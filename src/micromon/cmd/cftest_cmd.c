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


#include <ctype.h>
#include <stddef.h>
#include <string.h>
#include <machine.h>

#include "micromon.h"
#include "expr.h"

/*
 * cftest <drive> <sector>
 *
 * Checks that writing to a CF card works, without losing anything:
 * saves the sector, writes two test patterns to it (reading each back),
 * then puts the original contents back. <sector> is an absolute sector
 * number on the card, not relative to a partition.
 *
 * Only a power cut part way through would lose the sector's contents,
 * so pick one that isn't in use, e.g. one between the partition table
 * and the first partition.
 */

static uint8_t saved[CF_SECTOR_SIZE];
static uint8_t pattern[CF_SECTOR_SIZE];
static uint8_t readback[CF_SECTOR_SIZE];

static int get_value(const char *str, long *val) {
    int error_pos;

    if (expr_evaluate(str, val, &error_pos) != EXPR_OK) {
        printk("Couldn't evaluate expression: '%s'\n", str);
        return NOT_OK;
    }

    return OK;
}

static int write_and_check(uint8_t drive, uint32_t sector, const char *what) {
    if (cf_write(drive, sector, pattern) != OK) {
        printk("  %s: write FAILED\n", what);
        return NOT_OK;
    }

    if (cf_read(drive, sector, readback) != OK) {
        printk("  %s: read back FAILED\n", what);
        return NOT_OK;
    }

    for (int i=0; i<CF_SECTOR_SIZE; i++) {
        if (readback[i] != pattern[i]) {
            printk("  %s: MISMATCH at byte %d (wrote $%02X, read $%02X)\n", what, i, pattern[i], readback[i]);
            return NOT_OK;
        }
    }

    printk("  %s: ok\n", what);

    return OK;
}

void handle_cftest_command(int argc, char *argv[]) {
    long drive, sector;
    int failed = NO;

    if ((argc != 3) || (get_value(argv[1], &drive) != OK) || (get_value(argv[2], &sector) != OK)) {
        printk("usage: cftest <drive> <sector>\n");
        return;
    }

    if ((drive < 0) || (drive > 1) || (cf_drive_ready((uint8_t)drive) != YES)) {
        printk("No CF card in drive %d\n", drive);
        return;
    }

    if (cf_read((uint8_t)drive, (uint32_t)sector, saved) != OK) {
        printk("Couldn't read sector %d - nothing written\n", sector);
        return;
    }

    printk("Testing drive %d, sector %d:\n", drive, sector);

    // Pattern 1: alternating bits, with the sector number at the start
    for (int i=0; i<CF_SECTOR_SIZE; i++) {
        pattern[i] = (i & 1)?0xaa:0x55;
    }
    memcpy(pattern, &sector, sizeof(sector));

    if (write_and_check((uint8_t)drive, (uint32_t)sector, "pattern 1") != OK) {
        failed = YES;
    }

    // Pattern 2: the inverse, so every bit has been both 0 and 1
    for (int i=0; i<CF_SECTOR_SIZE; i++) {
        pattern[i] = ~pattern[i];
    }

    if ((failed == NO) && (write_and_check((uint8_t)drive, (uint32_t)sector, "pattern 2") != OK)) {
        failed = YES;
    }

    // Always put the original back
    memcpy(pattern, saved, CF_SECTOR_SIZE);

    if (write_and_check((uint8_t)drive, (uint32_t)sector, "restore") != OK) {
        printk("WARNING: sector %d may not hold its original contents\n", sector);
        failed = YES;
    }

    printk("%s\n", (failed == YES)?"CF write test FAILED":"CF write test passed");
}
