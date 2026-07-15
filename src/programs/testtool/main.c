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
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <machine.h>
#include <disk.h>
#include <ext2.h>
#include <nonstd.h>

#include "testtool.h"
#include "expr.h"

extern filesystem_t _mounted_filesystems[MAX_FILESYSTEMS];
extern uint8_t _num_mounted_filesystems;

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

int cf_present(void) {
    /* 
     * Poll the status register: if no card is present, this will
     * consistently return 0. If a card is present, DRDY and DSC
     * should be set.
     */ 
    register uint8_t cf_good = CF_ST_RDY | CF_ST_DSC;

    for (int i=0; i<1000; i++) {

        if ((*cf_reg_status & cf_good) == cf_good) {
            return 1;
        }
    }

    return 0;
}

void handle_command(int argc, char *argv[]) {
    register const char *cmd = argv[0];

    if (is_command(cmd, "quit", 1) == YES) {
        printf("\nBye.\n");
        exit(0);
    }
    else if (is_command(cmd, "dump", 2) == YES) {
        handle_dump_command(argc, argv);
    }
    else if (is_command(cmd, "eval", 2) == YES) {
        handle_eval_command(argc, argv);
    }
    else if (is_command(cmd, "rtc", 2) == YES) {
        handle_rtc_command(argc, argv);
    }
    else if (is_command(cmd, "show", 2) == YES) {
        handle_show_command(argc, argv);
    }
    else if (is_command(cmd, "vars", 2) == YES) {
        handle_vars_command(argc);
    }
    else if (is_command(cmd, "help", 2) == YES) {
        printf("Commands are:\n");
        printf("  dump [<address> [<count>]]\n");
        printf("  eval <expression>\n");
        printf("  rtc  time | erase\n");
        printf("  show [ bg <num> | inode <num> | super | block <num> ]\n");
        printf("  vars\n");
        printf("  quit\n\n");
    }
    else {
        printf("Unrecognised command.\n");
    }
}

int main(void) {
    bool_t running = YES;
    char cmd_line[MAX_LINE];

    if (!cf_present()) {
        printf("No CF drive(s) present.\n");
        exit(1);
    }

    if (_num_mounted_filesystems == 0) {
        printf("No valid filesystems found.\n");
        exit(2);
    }

    i2c_init();

    printf("TestTool v1.0.\n\n");
    
    while (running == YES) {
        printf("# ");

        if (gets(cmd_line) == NULL) {
            running = NO;
        }
        else {
            char *argv[MAX_ARGS];
            int argc = split_str(cmd_line, ' ', argv, MAX_ARGS);

            if (argc) {
                handle_command(argc, argv);
            }
        }
    }

    return 0;
}