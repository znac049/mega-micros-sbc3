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

#include "micromon.h"
#include "expr.h"

void handle_eval_command(int argc, char *argv[]) {
    char exp[MAX_LINE];
    expr_error_t res;
    long val;
    int error_pos;

    if (argc == 1) {
        kprintf("usage: eval <expression>\n");
        return;
    }

    exp[0] = EOS;

    for (int i=1; i<argc; i++) {
        strcat(exp, argv[i]);
        strcat(exp, " ");
    }

    res = expr_evaluate(exp, &val, &error_pos);
    if (res != EXPR_OK) {
        kprintf("Failed to parse expression '%s'\n%s\n\n", exp, expr_error_string(res));
    }
    else {
        kprintf("-> %d (0x%08x)\n", val, val);
    }
}