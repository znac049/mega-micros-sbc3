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
#include <nonstd.h>
#include <machine.h>
#include <setjmp.h>

#include "micromon.h"
#include "expr.h"

uint32_t go_address = 0x40000;

jmp_buf go_env;

void handle_go_command(int argc, char *argv[]) {
    long val;
    expr_error_t res;
    int error_pos;
    register const char *cmd = argv[1];
    int exit_code = 0;
    int (*fn)(void);
    int go_res;


    if ((argc == 2) && is_command(cmd, "help", 2) == YES) {
        kprintf("usage:\n");
        kprintf("  go [<address>]       run code.\n");
        return;
    }
    else if (argc == 2) {
        res = expr_evaluate(argv[1], &val, &error_pos);
        if (res != EXPR_OK) {
            kprintf("Couldn't evaluate expression: '%s'\n", argv[1]);
            return;
        }

        go_address = val;
    }

    go_res = setjmp(go_env);
    if (go_res == 0) {
        // First call - invoke the user program
        kprintf("Launching code at 0x%08x\n", go_address);
        fn = (int (*)(void))go_address;
        exit_code = fn();
        if (exit_code) {
            kprintf("Code exited with %d\n", exit_code);
        }
    }
    else {
        kprintf("User code failed with code %d\n", go_res);
    }
}