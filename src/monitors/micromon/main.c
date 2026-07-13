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

#define MAX_LINE 512
#define MAX_ARGS 32

static int major = 0;
static int minor = 1;
static char *tag = "alpha";

static uint32_t saved_dump_address = 0;
static size_t saved_dump_count = 512;

void setup(void) {
    uint8_t *ram_end = (uint8_t *)0x3fffff; //get_ram_size();

    _claim_pit();

    setup_duart();
    set_isr_handler(32, (unsigned int)trap0_handler);

    kprintf("Mega-Micros SBC-3 Computer System\n");
    kprintf("MicroMon v%d.%d-%s starting.\n", major, minor, tag);
    kprintf("RAM: 0x00000000 - 0x%08x\n", ram_end);

}

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

void do_dump(int argc, char *argv[]) {
    if (argc > 3) {
        kprintf("usage: dump [<start_address> [<count>]]\n");
        return;
    }

    if (argc == 2) {
        saved_dump_address = strtol(argv[1], NULL, 10);
    }

    if (argc == 3) {
        saved_dump_count = strtol(argv[2], NULL, 10);
    }

    dump((uint8_t *)saved_dump_address, saved_dump_count, NO);
}

void handle_command(int argc, char *argv[]) {
    register const char *cmd = argv[0];

    argc += 0;

    if (is_command(cmd, "quit", 1) == YES) {
        kprintf("\nYou can't quit from the monitor!.\n");
    }
    else if (is_command(cmd, "help", 2) == YES) {
        kprintf("Commands are:\n");
        kprintf("  dump [<start_address> [<count>]]\n");
        kprintf("  quit\n\n");
    }
    else if (is_command(cmd, "dump", 2) == YES) {
        do_dump(argc, argv);
    }
    else {
        kprintf("Unrecognised command.\n");
    }
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