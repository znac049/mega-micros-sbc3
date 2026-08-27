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

uint32_t go_address = 0x40000;

jmp_buf go_env;

static int do_the_business(int argc, char *argv[]) {
    int (*fn)(const int, const char *[]) = (int (*)(const int, const char *[]))go_address;
    int go_res;
    
    go_res = setjmp(go_env);
    if (go_res == 0) {
        int return_code;
    
        // First call - invoke the user program
        kprintf("Launching code at 0x%08x\n", go_address);
        return_code = fn(argc, (const char **)argv);
        if (return_code) {
            kprintf("\n---\nUser code exited with %d\n", return_code);
        }
        else {
            kprintf("\n---\nExited.\n");
        }

        return return_code;
    }
    else {
        // If we arrive here, it's because something has gone wrong
        kprintf("\n---\nUser code exited with code %d\n", go_res-1);

        return go_res - 1;
    }
}

void handle_go_command(int argc, char *argv[]) {
    long val;
    expr_error_t res;
    int error_pos;

    kprintf("Args are:\n");
    for (int i=0; i<argc; i++) {
        kprintf("%d: '%s'\n", i, argv[i]);
    }

    // Lose the 'go' arg
    argc--;
    argv++;

    /* 
     * Possibilities:
     *   go
     *   go <address>
     *   go -- <args>
     *   go <address> <args>
     */
    if (argc >= 1) {
        if (strcmp(argv[0], "--") == 0) {
            argc--;
            argv++;
        }
        else {
            // if it evaluates successfully, assume its a start address
            res = expr_evaluate(argv[1], &val, &error_pos);
            // If it didn't evaluate, assume it's part of the program's arguments
            if (res == EXPR_OK) {
                // We're treating it as the start address, so don't pass it to the user code
                argc--;
                argv++;

                go_address = val;
            }
        }
    }

    kprintf("Calling with ArgC=%d:\n", argc);
    for (int i=0; i<argc; i++) {
        kprintf("%d: '%s'\n", i, argv[i]);
    }

    do_the_business(argc, argv);
}