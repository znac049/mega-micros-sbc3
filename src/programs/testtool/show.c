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

void handle_show_command(int argc, char *argv[]) {
    register const char *cmd = argv[1];

    if ((argc == 1) || ((argc == 2) && is_command(cmd, "help", 2) == YES)) {
        printf("show accepts the following sub-commands:\n");
        printf("  bg <num>          show info about Block group <num>\n");
        printf("  block <num>       dump disk block <num>\n");
        printf("  inode <num>       dump inode <num>\n");
        printf("  rtc [nvram]       show rtc related data\n");
        printf("  super             dump the primary superblock\n");
        printf("\n");
    }
    else if (is_command(cmd, "bg", 0) == YES) {
        handle_bginfo_subcommand(argc-1, &argv[1]);
    }
    else if (is_command(cmd, "block", 2) == YES) {
        handle_block_subcommand(argc-1, &argv[1]);
    }
    else if (is_command(cmd, "inode", 2) == YES) {
        handle_inode_subcommand(argc-1, &argv[1]);
    }
    else if (is_command(cmd, "rtc", 2) == YES) {
        handle_rtc_subcommand(argc-1, &argv[1]);
    }
    else if (is_command(cmd, "super", 2) == YES) {
        handle_sbinfo_subcommand(argc-1);
    }
}

