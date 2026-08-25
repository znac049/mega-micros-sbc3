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

#include <stdio.h>
#include <ctype.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <nonstd.h>
#include <machine.h>

#include "micromon.h"
#include "expr.h"


void print_help(int argc, char *argv[]);

static command_t commands[] = {
    {"help",        2, print_help},
    {"cat",         0, handle_cat_command},
    {"dir",         0, handle_dir_command},
    {"disassemble", 3, handle_disasm_command},
    {"dump",        2, handle_dump_command},
    {"eval",        2, handle_eval_command},
    {"go",          0, handle_go_command},
    {"load",        2, handle_load_command},
    {"probe",       3, handle_probe_command},
    {"pwd",         0, handle_pwd_command},
    {"rtc",         3, handle_rtc_command},
    {"usb1",        0, handle_usb_command},
    {"usb2",        0, handle_usb_command},
};

#define NUM_COMMANDS (sizeof(commands)/sizeof(command_t))

bool_t is_command(const char *cmd, const char *target, int min_target_len) {
    int cmd_len = strlen(cmd);
    int target_len = strlen(target);

    if (min_target_len == 0) {
        min_target_len = target_len;
    }

    if ((cmd_len > target_len) || (cmd_len < min_target_len)) {
        return NO;
    }

    for (int i=0; i<cmd_len; i++) {
        if (tolower(cmd[i]) != tolower(target[i])) {
            return NO;
        }
    }

    return YES;
}

bool_t is_blank(const char *str) {
    while (*str) {
        if (!isblank(*str++)) {
            return NO;
        }
    }

    return YES;
}

void print_help(int argc, char *argv[]) {
    argc--;
    argv++;

    kprintf("Commands are:\n");
    kprintf("  cat <filename>\n");
    kprintf("  disassemble <address>\n");
    kprintf("  dump [<start_address> [<count>]]\n");
    kprintf("  eval <expression>\n");
    kprintf("  go <address>]\n");
    kprintf("  load\n");
    kprintf("  probe\n");
    kprintf("  rtc (erase) | (time [hh:mm[:ss]]) | (date [yyyy:mm:dd])\n");
    kprintf("  usb1|2 baud <baudrate>\n");
    kprintf("  quit\n\n");
}

void handle_command(int argc, char *argv[]) {
    register const char *cmd = argv[0];

    argc += 0;

    if (is_blank(cmd) == YES) {
        return;
    }

    for (unsigned int i=0; i<NUM_COMMANDS; i++) {
        if (is_command(cmd, commands[i].command, commands[i].min_required) == YES) {
            // call the handler
            commands[i].handler(argc, argv);
            return;
        }
    }

    kprintf("'%s': Unrecognised command.\n", argv[0]);
}

void main(void) {
    char cmd_line[MAX_LINE];

    setup();

    while (1) {
        kprintf("# ");

        if (kgets(cmd_line) != NULL) {
            char *argv[MAX_ARGS];
            int argc = split_str(cmd_line, ' ', argv, MAX_ARGS);

            if (argc) {
                handle_command(argc, argv);
            }
        }
    }
}