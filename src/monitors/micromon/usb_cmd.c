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

static void do_baud_cmd(int argc, char *argv[]) {
    uint32_t baud;
    expr_error_t res;
    int error_pos;

    if (argc != 3) {
        kprintf("usage: %s baud <baudrate>\n", argv[0]);
        return;
    }

    res = expr_evaluate(argv[2], (long *)&baud, &error_pos);
    if (res == EXPR_OK) {
        int port = 0;

        if (strcasecmp(argv[0], "usb2") == 0) {
            port = 1;
        }

        kprintf("Setting baudrate on %s to %d...\n", argv[0], baud);
        if (bios_set_baud(port, baud) == NOT_OK) {
            kprintf("FAILED.\n");
        }
        else {
            kprintf("Success.\n");
        }
    }
    else {
        kprintf("Couldn't evaluate baud rate expression: '%s'\n", argv[2]);
        return;
    }
}

void handle_usb_command(int argc, char *argv[]) {
    register const char *cmd = argv[1];

    if ((argc == 1) || ((argc == 2) && is_command(cmd, "help", 2) == YES)) {
        kprintf("%s accepts the following sub-commands: \n", argv[0]);
        kprintf("  baud <baudrate>       set the baudrate\n");
        kprintf("\n");
    }
    else if (is_command(cmd, "baud", 2) == YES) {
        do_baud_cmd(argc, argv);
    }
    else {
        kprintf("'%s' is not a valid sub-command.\n", argv[1]);
    }
}