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
#define TWO_MEG (ONE_MEG*2)
#define THREE_MEG (ONE_MEG*3)
#define FOUR_MEG (ONE_MEG*4)
#define FIVE_MEG (ONE_MEG*5)
#define EIGHT_MEG (ONE_MEG*8)

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
 * This isn't as simple as attempting to access possible RAM locations and
 * handling bes error to detect end of RAM as it looks like the CPLD generates
 * DTACK for the entire 8MB possible ram.
 *
 * But hey, let's simplify it for the SBC-3 board which only supports three
 * memory configurations: 2MB, 4MB or 8MB and since the CPLD decodes the 
 * whole address space, you just need top write to a few locations and see what
 * you read back from it. If yopu get what you wrote, then it's 8MB, but lower
 * amounts mirror what was written to lower addresses
 */
uint32_t get_ram_end(void) {
    volatile uint8_t *mem = (uint8_t *)0;

    mem[ONE_MEG] = 1;
    mem[THREE_MEG] = 3;
    mem[FIVE_MEG] = 5;

    mem[TWO_MEG] = 0xff;

    if ((mem[ONE_MEG] == 1) && (mem[THREE_MEG] == 3) && (mem[FIVE_MEG] == 5)) {
        return EIGHT_MEG-1;
    }
    else if ((mem[ONE_MEG] == 1) && (mem[THREE_MEG] == 3)) {
        return FOUR_MEG-1;
    }

    return TWO_MEG-1;
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