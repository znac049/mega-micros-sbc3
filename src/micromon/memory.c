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

#define RAM_MAX ((1024*1024*8)-1)
#define ONE_MEG (1024*1024)

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

/*
 * This isn't as simple as ayttempting to access possible RAM locations and
 * handling bes error to detect end of RAM as it looks like the CPLD generates
 * DTACK for the entire 8MB possible ram.
 */
uint32_t get_ram_end(void) {
    for (uint32_t addr=(ONE_MEG-1); addr<=RAM_MAX; addr+=ONE_MEG) {
        if (check_address((uint16_t *)addr) == 0) {
            return addr;
        }
    }

    return 0x007fffff;
}

int test_ram(uint32_t start_addr, uint32_t end_addr) {
    int i = 0;

    if (start_addr & 1)
        start_addr++;
    
    if (end_addr && 1) 
        end_addr--;

    printk(" Testing......");
    for (uint32_t addr=start_addr; addr<end_addr; addr += 2) {
        if (!check_address((uint16_t *)addr)) {
            printk("\nMemory test failed at $%08x\n", addr);
            return addr;
        }

        if (++i == 40000) {
            i = 0;
            spin();
        }
    }

    for (int j=0; j<5; j++) {
        printk("%c", BS);
    }

    printk(": PASS          \n");

    return OK;
}