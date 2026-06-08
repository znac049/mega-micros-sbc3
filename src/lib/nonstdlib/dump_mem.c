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
#include <ctype.h>

void dump_mem(uint8_t *buf, size_t count, uint8_t print_zeroes) {
    int all_zeroes = 0;
    int printed_something = 0;

    printf("Memory at 0x%08x\n", buf);

    for (size_t i=0; i<count; i+=16) {
        all_zeroes = 1;
        for (int x=0; x<16; x++) {
            if (buf[i+x]) {
                all_zeroes = 0;
            }
        }

        if (!all_zeroes || print_zeroes) {
            printf("%04x: ", i);
            for (int x=0; x<16; x++) {
                printf("%02x ", buf[i+x]);
            }

            printf("    ");
            for (int x=0; x<16; x++) {
                char c = buf[i+x];

                if ((c < ' ') || (c > '_'))
                    c = '.';

                printf("%c", c);
            }
            printf("\n");

            printed_something = 1;
        }
    }

    if (!printed_something) {
        printf("Data is all zeroes.\n");
    }

    printf("\n");
}