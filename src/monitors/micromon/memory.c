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

#define SKIP_SIZE 4096

uint32_t get_ram_size(void) {
    uint8_t *test_addr = (uint8_t *)SKIP_SIZE;
    uint8_t *last_good_addr = (uint8_t *)-1;
    int val = peek(test_addr);

    while (val != -1) {
        last_good_addr = test_addr;
        test_addr += SKIP_SIZE;
        val = peek(test_addr);
    }

    for (int i=0; i<SKIP_SIZE; i++) {
        if (peek(last_good_addr) == -1) {
            last_good_addr--;

            return (uint32_t)last_good_addr;
        }

        last_good_addr++;
    }

    return (uint32_t)last_good_addr;
}