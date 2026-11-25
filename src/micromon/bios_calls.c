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
#include <string.h>
#include <extras.h>
#include <machine.h>
#include <setjmp.h>

#include "micromon.h"

/*
 * BIOS_TIME: seconds since 1 January 1970, from the RTC; 0 if there isn't
 * one (or it doesn't hold a sensible time). Also stored in *t if t isn't
 * NULL, as time() does.
 */
time_t bios_time(time_t *t) {
    time_t now = (time_t)ext2_now();

    if (t != NULL) {
        *t = now;
    }

    return now;
}

int bios_exit(int exit_code, pid_t pid) {
    // Free up any malloc'd items currently on the heap
    clean_heap(pid);

    // Pass control back to the monitor via the 'go' command handler
    longjmp(go_env, exit_code);         // everyone loves a goto, right!
}

int bios_test(int p1, int p2, int p3) {
    printk("bios_test: p1=%08x, p2=%08x, p3=%08x\n", p1, p2, p3);

    return 42;
}