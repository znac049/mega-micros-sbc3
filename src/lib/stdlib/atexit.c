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

/*
 * Functions to call when the program ends: those given to atexit(), and
 * the destructors of C++'s static objects, which the compiler registers
 * with __cxa_atexit() as each one is made. exit() calls them, last
 * registered first, then the .fini_array functions (the C destructors
 * and any older-style C++ ones).
 */

#include <stdlib.h>
#include <stddef.h>

#define MAX_EXIT_HANDLERS 64    // C requires at least 32

// One of the two: a C++ destructor, with its object, or an atexit() function
typedef struct {
    void (*fn)(void *);
    void *arg;
    void (*plain_fn)(void);
} exit_handler_t;

static exit_handler_t exit_handlers[MAX_EXIT_HANDLERS];
static int num_exit_handlers;

// Identifies the program to __cxa_atexit(). There are no shared
// libraries, so its value doesn't matter, only that it exists.
void *__dso_handle = &__dso_handle;

int __cxa_atexit(void (*fn)(void *), void *arg, void *dso);

int __cxa_atexit(void (*fn)(void *), void *arg, void *dso) {
    (void)dso;

    if (num_exit_handlers >= MAX_EXIT_HANDLERS) {
        return NOT_OK;
    }

    exit_handlers[num_exit_handlers].fn = fn;
    exit_handlers[num_exit_handlers].arg = arg;
    exit_handlers[num_exit_handlers].plain_fn = NULL;
    num_exit_handlers++;

    return OK;
}

int atexit(void (*fn)(void)) {
    if (num_exit_handlers >= MAX_EXIT_HANDLERS) {
        return NOT_OK;
    }

    exit_handlers[num_exit_handlers].fn = NULL;
    exit_handlers[num_exit_handlers].arg = NULL;
    exit_handlers[num_exit_handlers].plain_fn = fn;
    num_exit_handlers++;

    return OK;
}

// From the link script
extern void (*__fini_array_start[])(void);
extern void (*__fini_array_end[])(void);

void _run_exit_handlers(void) {
    // A handler may itself call atexit(), so take them one at a time
    while (num_exit_handlers > 0) {
        exit_handler_t *h = &exit_handlers[--num_exit_handlers];

        if (h->plain_fn != NULL) {
            h->plain_fn();
        }
        else {
            h->fn(h->arg);
        }
    }

    for (size_t i = __fini_array_end - __fini_array_start; i > 0; i--) {
        __fini_array_start[i - 1]();
    }
}
