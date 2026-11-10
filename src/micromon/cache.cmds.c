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
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <machine.h>
#include <errno.h>

#include "micromon.h"

/* Cache Control Registers (CACR) bits */
#define CA_EI       (1<<0)
#define CA_FI       (1<<1)
#define CA_CI       (1<<3)
#define CA_IBE      (1<<4)
#define CA_ED       (1<<8)
#define CA_FD       (1<<9)
#define CA_CD       (1<<11)
#define CA_DBE      (1<<12)
#define CA_WA       (1<<13)

void handle_icache_cmd(int argc, char *argv[]) {
    int target;
    uint32_t cacr = get_cacr();

    if (argc == 1) {
        printk("CACR=$%08x\n", cacr);

        return;
    }

    if (argc != 2) {
        printk("usage: %s [ <on> | <off> ]\n", argv[0]);
        return;
    }

    target = parse_truth(argv[1]);
    if (target == NOT_OK) {
        printk("please supply one of 'on', 'off', 'enable' or 'disable'\n");
        return;
    }

    if (target == YES) {
        /* We want to enable the cache */
        cacr &= CA_EI;
    }
    else {
        /* we want to disable the cache */
        cacr |= ~CA_EI;     /* Disable */
        cacr |= CA_CI;      /* Clear */
    }

    set_cacr(cacr);
}

void handle_dcache_cmd(int argc, char *argv[]) {
    int target;
    uint32_t cacr = get_cacr();

    if (argc == 1) {
        printk("CACR=$%08x\n", cacr);

        return;
    }

    if (argc != 2) {
        printk("usage: %s [ <on> | <off> ]\n", argv[0]);
        return;
    }

    target = parse_truth(argv[1]);
    if (target == NOT_OK) {
        printk("please supply one of 'on', 'off', 'enable' or 'disable'\n");
        return;
    }

    if (target == YES) {
        /* We want to enable the cache */
        cacr &= CA_ED;
    }
    else {
        /* we want to disable the cache */
        cacr |= ~CA_ED;     /* Disable */
        cacr |= CA_CD;      /* Clear */
    }

    set_cacr(cacr);
}

