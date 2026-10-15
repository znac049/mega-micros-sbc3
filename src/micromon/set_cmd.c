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

#include "micromon.h"

static long edit_address = 0;

static char *skip_first_word(char *s) {
    return s;
}

static void do_set(char *cmdline, loc_size_t access_size) {
    char *argv[MAX_ARGS];
    int argc;
    char *cp;
    
    printk("do_set: '%s'\n, cmdline");

    if (*cmdline == '@') {
        cmdline++;

        switch (*cmdline) {
            case 'b':
                cmdline++;
                break;

            case 'w':
                access_size = WORD;
                cmdline++;
                break;

            case 'l':
                access_size = LONG;
                cmdline++;
                break;
        }
    }
    else {
        cmdline = skip_first_word(cmdline);
    }

    /* Is there an equals symbol present? */
    cp = strchr(cmdline, '=');
    if (cp != NULL) {
        expr_error_t res;
        long val;
        int error_pos;

        *cp++ = EOS;
        printk("do_set: address supplied is %s\n", cmdline);

        res = expr_evaluate(cmdline, &val, &error_pos);
        if (res != EXPR_OK) {
            printk("Failed to parse address '%s'\n%s\n\n", cmdline, expr_error_string(res));
            return;
        }

        edit_address = val;

        cmdline = cp;
    }

    argc = split_str(cmdline, ',', argv, MAX_ARGS);
    
    if (argc > 0) {
        printk("$%08x ->", edit_address);
    }

    for (int i=0; i<argc; i++) {
        expr_error_t res;
        long val;
        int error_pos;

        printk("do_set: expr='%s'\n", argv[i]);

        res = expr_evaluate(argv[i], &val, &error_pos);
        if (res != EXPR_OK) {
            printk("Failed to parse address '%s'\n%s\n\n", argv[i], expr_error_string(res));
            return;
        }

        switch (access_size) {
            case BYTE:
                {
                    uint8_t *mem = (uint8_t *)edit_address;

                    val &= 0xff;
                    *mem = (uint8_t)val;
                    printk(" $%02x", val);
                    edit_address++;
                }
                break;

            case WORD:
                {
                    uint16_t *mem = (uint16_t *)edit_address;

                    val &= 0xffff;
                    *mem = (uint16_t)val;
                    printk(" $%04x", val);
                    edit_address += 2;
                }
                break;

            case LONG:
                {
                    uint32_t *mem = (uint32_t *)edit_address;

                    *mem = (uint32_t)val;
                    edit_address += 4;
                    printk(" $%08x", val);
                }
                break;
        }
    }

    if (argc > 0) {
        printk("\n");
    }
}

void handle_setbyte_command(char *cmdline) {
    do_set(cmdline, BYTE);
}

void handle_setword_command(char *cmdline) {
    do_set(cmdline, WORD);
}

void handle_setlong_command(char *cmdline) {
    do_set(cmdline, LONG);
}
