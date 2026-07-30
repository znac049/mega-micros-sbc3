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

void dump(uint8_t *buf, size_t count, uint8_t print_zeroes, const char *heading, bool_t absolute_addresses) {
    int all_zeroes = 0;
    int printed_something = 0;

    if (heading != NULL) {
        kprintf("%s:\n", heading);
    }
    else {
        kprintf("Memory at 0x%08x\n", buf);
    }

    for (size_t i=0; i<count; i+=16) {
        int needed = 16;
        int skip = 0;

        if ((i + 16) > count) {
            needed = count - i;
            skip = 16 - needed;
        }

        if (print_zeroes) {
            all_zeroes = 0;
        }
        else {
            all_zeroes = 1;
            for (int x=0; x<needed; x++) {
                if (buf[i+x]) {
                    all_zeroes = 0;
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

            for (int x=0; x<needed; x++) {
                kprintf("%02x ", buf[i+x]);
            }

            for (int x=0; x<skip; x++) {
                kprintf("   ");
            }

            kprintf("    ");
            for (int x=0; x<needed; x++) {
                char c = buf[i+x];

                if ((c < ' ') || (c > '_'))
                    c = '.';

                kprintf("%c", c);
            }
            kprintf("\n");

            printed_something = 1;
        }
    }

    if (!printed_something) {
        kprintf("Data is all zeroes.\n");
    }

    kprintf("\n");
}