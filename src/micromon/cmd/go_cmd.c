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
#include "expr.h"

// Where 'go' starts a program: _PROGRAM_BASE (memory.ld), until one is loaded
extern char _PROGRAM_BASE[];
uint32_t go_address = (uint32_t)_PROGRAM_BASE;

jmp_buf go_env;

/*
 * Each program gets a stack of its own from the heap, rather than running
 * on the monitor's. Its BIOS calls and the interrupts that happen while it
 * runs use it too: they need up to about 5K. A program can ask for a
 * different size by linking with -z stack-size=<bytes>.
 */
#define DEFAULT_STACK_SIZE  (64 * 1024)
#define MIN_STACK_SIZE      (8 * 1024)

/*
 * The bottom of the stack is filled with this when the program starts. If
 * any of it has changed when the program ends, the stack (very probably)
 * overflowed.
 */
#define STACK_GUARD_LONGS   256
#define STACK_GUARD_FILL    0x5354414bU             // "STAK"

static int do_the_business(int argc, char *argv[]) {
    uint32_t stack_size = (program_stack_size != 0)?program_stack_size:DEFAULT_STACK_SIZE;
    uint32_t *volatile stack;
    volatile int return_code;
    int go_res;

    stack_size = (stack_size + 3) & ~3U;
    if (stack_size < MIN_STACK_SIZE) {
        stack_size = MIN_STACK_SIZE;
    }

    // The monitor owns it, so it's still there to check after the program's exit()
    stack = (uint32_t *)bios_malloc(stack_size, MONITOR_PID);
    if (stack == NULL) {
        printk("Not enough memory for the program's stack (%u bytes)\n", (unsigned)stack_size);
        return -1;
    }

    for (int i=0; i<STACK_GUARD_LONGS; i++) {
        stack[i] = STACK_GUARD_FILL;
    }

    go_res = setjmp(go_env);
    if (go_res == 0) {
        // First call - invoke the user program
        // printk("Launching code at 0x%08x\n", go_address);
        return_code = call_on_stack(go_address, argc, argv, (uint8_t *)stack + stack_size);
        if (return_code) {
            printk("\n---\nSHOULDN'T HAPPEN: User code exited with %d\n", return_code);
        }
        else {
            printk("\n---\nSHOULDN'T HAPPEN: Exited.\n");
        }
    }
    else {
        // If we arrive here, it's because either:
        //   1. something has gone wrong
        //   2. the user code has called exit() (most likely)
        // printk("\n---\nUser code exited with code %d\n", go_res-1);

        return_code = go_res - 1;
    }

    for (int i=0; i<STACK_GUARD_LONGS; i++) {
        if (stack[i] != STACK_GUARD_FILL) {
            printk("\n*** The program used all, or nearly all, of its %u byte stack (link it with -z stack-size=<bytes> for more)\n",
                (unsigned)stack_size);
            break;
        }
    }

    bios_free(stack, MONITOR_PID);

    return return_code;
}

int go_run(int argc, char *argv[]) {
    int res = do_the_business(argc, argv);

    // Close anything the program left open, so its writes are flushed
    vfs_close_all();

    return res;
}

void handle_go_command(int argc, char *argv[]) {
    long val;
    expr_error_t res;
    int error_pos;

    argv[0] = program_name;
    // printk("Args are:\n");
    // for (int i=0; i<argc; i++) {
    //     printk("%d: '%s'\n", i, argv[i]);
    // }

    /* 
     * Possibilities:
     *   go
     *   go <address>
     *   go -- <args>
     *   go <address> <args>
     */
    if (argc >= 1) {
        if (strcmp(argv[1], "--") == 0) {
            for (int i = 2; i<argc; i++) {
                argv[i-1] = argv[i];
            }
            argc--;
        }
        else if (argc == 2) {
            // if it evaluates successfully, assume its a start address
            res = expr_evaluate(argv[1], &val, &error_pos);
            // If it didn't evaluate, assume it's part of the program's arguments
            if (res == EXPR_OK) {
                // We're treating it as the start address, so don't pass it to the user code
                for (int i = 2; i<argc; i++) {
                    argv[i-1] = argv[i];
                }
                argc--;

                go_address = val;
            }
        }
    }

    // printk("Calling with ArgC=%d:\n", argc);
    // for (int i=0; i<argc; i++) {
    //     printk("%d: '%s'\n", i, argv[i]);
    // }

    go_run(argc, argv);
}