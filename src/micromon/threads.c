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

#include "threads.h"

static thread_t threads[MAX_THREADS];
static int num_threads = 0;
static int running_thread = 0;

inline static void set_sp(int new_sp) {
    __asm__ __volatile__
    (
        "move.l %0,a7\n\t"
        :
        : "g"(new_sp)
        :
    );
}

inline static int get_sp(void) {
    static uint32_t res;

    __asm__ __volatile__
    (
        "move.l %%a7,%[res]\n\t"
        : [res] "=r"(res)
        : 
        :
    );

    return res;
}

int init_threads(void) {
    return OK;
}

int create_thread(int (*fn)(void), uint32_t initial_stack) {
    thread_t *tp;
    uint32_t sp;

    if (num_threads >= MAX_THREADS) {
        return NOT_OK;
    }

    tp = &threads[num_threads++];


    sp = get_sp();

    // Save the "return address"
    set_sp(initial_stack);

    tp->sp = get_sp();

    set_sp(sp);
    fn();

    running_thread++;

    return OK;
}