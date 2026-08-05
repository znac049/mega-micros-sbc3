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

static volatile int berr = NO;

static ISR bus_error(void) {
    berr = YES;
}

static int get_byte(uint8_t *addr) {
    uint8_t byte_val;

    berr = NO;
    byte_val = *addr;

    return (berr==YES)?-1:byte_val;
}

void dump(uint8_t *buf, size_t count, uint8_t print_zeroes, const char *heading, bool_t absolute_addresses) {
    int all_zeroes = NO;
    int printed_something = 0;
    unsigned int old_handler = set_isr_handler(VEC_BUS_ERROR, (unsigned int)bus_error);
    int byte_val;


    if (heading != NULL) {
        kprintf("%s:\n", heading);
    }
    else {
        kprintf("Memory at 0x%08x\n", buf);
    }

    for (uint32_t i=0; i<count; i+=16) {
        uint32_t needed = 16;
        int skip = 0;

        if ((i + 16) > count) {
            needed = count - i;
            skip = 16 - needed;
        }

        if (print_zeroes) {
            all_zeroes = NO;
        }
        else {
            all_zeroes = YES;
            for (uint32_t x=0; x<needed; x++) {
                byte_val = get_byte(buf+i+x);
                if (byte_val > 0) {
                    all_zeroes = NO;
                    x = needed;
                }
            }
        }

        if (!all_zeroes || print_zeroes) {
            if (absolute_addresses == YES) {
                kprintf("=%08x: ", buf+i);
            }
            else {
                if (count <= 256) {
                    kprintf("+%02x: ", i);
                }
                else if (count < 65536) {
                    kprintf("+%04x: ", i);
                }
                else {
                    kprintf("+%08x: ", i);
                }
            }

            for (uint32_t x=0; x<needed; x++) {
                byte_val = get_byte(buf+i+x);

                if (byte_val == NOT_OK) {
                    // Bus Error occurred
                    kprintf("!! ");
                }
                else {
                    kprintf("%02x ", byte_val);
                }
            }

            for (int x=0; x<skip; x++) {
                kprintf("   ");
            }

            kprintf("    ");
            for (uint32_t x=0; x<needed; x++) {
                byte_val = get_byte(buf+i+x);

                if ((byte_val < ' ') || (byte_val > '_'))
                    byte_val = '.';

                if (byte_val == NOT_OK) {
                    kprintf(" ");
                }
                else {
                    kprintf("%c", byte_val);
                }
            }
            kprintf("\n");

            printed_something = 1;
        }
    }

    if (!printed_something) {
        kprintf("Data is all zeroes.\n");
    }

    kprintf("\n");

    set_isr_handler(VEC_BUS_ERROR, old_handler);
}