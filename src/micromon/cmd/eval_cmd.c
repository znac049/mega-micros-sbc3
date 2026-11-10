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
#include <machine.h>
#include <extras.h>

#include "micromon.h"
#include "expr.h"

void handle_eval_command(char *cmdline) {
    char exp[MAX_LINE];
    int argc;
    char *argv[MAX_ARGS];

    exp[0] = EOS;

    if (cmdline[0] == '=') {
        strcpy(exp, &cmdline[1]);
    }
    else {
        char *s = cmdline;

        while (!isspace(*s)) {
            s++;
        }

        while (isspace(*s)) {
            s++;
        }

        strcpy(exp, s);
    }

    argc = split_str(exp, ',', argv, MAX_ARGS);
    for (int i=0; i<argc; i++) {
        expr_error_t res;
        long val;
        int error_pos;

        // printk("to_eval: '%s'\n", argv[i]);

        res = expr_evaluate(argv[i], &val, &error_pos);
        if (res != EXPR_OK) {
            printk("Failed to parse expression '%s'\n%s\n\n", argv[i], expr_error_string(res));
        }
        else {
            printk("%s => %d (0x%08x)\n", argv[i], val, val);
        }
    }
}