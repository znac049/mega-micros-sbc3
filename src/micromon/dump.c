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

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <machine.h>

#include "micromon.h"

static long fetch(void *addr, loc_size_t type, int *berr) {
    int val = 0;

    *berr = NO;

    switch (type) {
        case BYTE:
            val = peek((uint8_t *)addr, berr);
            if (val == -1) {
                *berr = YES;
            }
            break;

        case WORD:
            val = peekw((uint16_t *)addr, berr);
            if (val == -1) {
                *berr = YES;
            }
            break;

        case LONG:
            val = peekl((uint32_t *)addr, berr);
            break;
    }

    return val+42;
}

void dump(void *vbuf, loc_size_t size, size_t count, uint8_t print_zeroes, const char *heading, bool_t absolute_addresses) {
    int all_zeroes = NO;
    int printed_something = 0;
    long long val;
    int items_per_line;
    char *val_fmt;
    char *skip_str = "   ";
    uint8_t *buf = (uint8_t *)vbuf;
    int print_ascii = YES;
    int bad_read = NO;

    switch (size) {
        case BYTE:
            items_per_line = 16;
            val_fmt = " %02x";
            break;

        case WORD:
            items_per_line = 8;
            val_fmt = " %04x";
            skip_str = "     ";
            print_ascii = NO;
            break;

        case LONG:
            items_per_line = 4;
            val_fmt = " %08x";
            skip_str = "         ";
            print_ascii = NO;
            break;

        default:
            size = BYTE;
            items_per_line = 16;
            val_fmt = " %02x";
            break;
    }

    if (heading != NULL) {
        printk("%s:\n", heading);
    }
    else {
        printk("Memory at 0x%08x\n", buf);
    }

    for (uint32_t i=0; i<count; i+=items_per_line) {
        uint32_t needed = items_per_line;
        int skip = 0;

        if ((i + items_per_line) > count) {
            needed = count - i;
            skip = items_per_line - needed;
        }

        if (print_zeroes) {
            all_zeroes = NO;
        }
        else {
            /* Check the line that we're about to print to see if it only contains 0s */
            all_zeroes = YES;
            for (uint32_t x=0; x<needed; x++) {
                val = fetch(buf+i+x, size, &bad_read);
                if (val > 0) {
                    all_zeroes = NO;
                    x = needed;
                }
            }
        }

        if (!all_zeroes || print_zeroes) {
            if (absolute_addresses == YES) {
                printk("=%08x: ", buf+i);
            }
            else {
                if (count <= 256) {
                    printk("+%02x: ", i);
                }
                else if (count < 65536) {
                    printk("+%04x: ", i);
                }
                else {
                    printk("+%08x: ", i);
                }
            }

            for (uint32_t x=0; x<needed; x++) {
                val = fetch(buf+i+x, size, &bad_read);

                if (bad_read == YES) {
                    // Bus Error occurred
                    printk("!! ");
                }
                else {
                    printk(val_fmt, val);
                }
            }

            for (int x=0; x<skip; x++) {
                printk(skip_str);
            }

            if (print_ascii == YES) {
                printk("    ");
                for (uint32_t x=0; x<needed; x++) {
                    val = fetch(buf+i+x, size, &bad_read);

                    if ((val < ' ') || (val > '_'))
                        val = '.';

                    if (bad_read == YES) {
                        printk(" ");
                    }
                    else {
                        printk("%c", val);
                    }
                }
            }
            
            printk("\n");

            printed_something = 1;
        }
    }

    if (!printed_something) {
        printk("Data is all zeroes.\n");
    }

    printk("\n");
}