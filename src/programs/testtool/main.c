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

static const char *dayNames[8] = {"?", "Sunday", "Monday", "Tuesday", "Wednesday","Thursday", "Friday", "Saturday"};

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

void do_rtc(int argc, char *argv[]) {
    ds1307_time_t t;
    int status;
 
    if ((argc < 1) || (argc > 2)) {
        printf("usage: show rtc [nvram]\n");
        return;
    }

    if ((argc > 1) && (is_command(argv[1], "help", 2) == YES)) {
        printf("show rtc accepts the following sub-commands:\n");
        printf("  <no arg>              read and print the time.\n");
        printf("  nvram                 dump the contents of the nvram (56 bytes)\n");
    }
    else if (argc == 1) {
        // Display the date and time
        status = ds1307_read_time(&t);
        if (status < 0) {
            printf("DS1307: no ACK from device -- check address/wiring/pull-ups\n");
        }
    
        printf("DS1307 date and time: %s 20%02u-%02u-%02u %02u:%02u:%02u\n",
            dayNames[t.day], t.year, t.month, t.date,
            t.hours, t.minutes, t.seconds);
    
        if (status == 1)
            printf("Warning: clock-halt (CH) bit is set -- oscillator is "
                "stopped, time above is not advancing until the clock "
                "is (re)started.\n");
    }
    else if (is_command(argv[1], "nvram", 2) == YES) {
        uint8_t nvram[56];

        if (ds1307_read_nvram(0, nvram, 56) != 56) {
            printf("Failed to read RTC nvram!\n");
            return;
        }

        printf("RTC nvram:\n");
        dump_mem(nvram, 56, YES);
    }
 
}

void do_sbinfo(int argc) {
    ext2_fs_t *fs = _mounted_filesystems[0].fs;

    if (argc != 1) {
        printf("usage: show super\n");
        return;
    }

    dump_ext2_sb(fs->sb);
}

void do_bginfo(int argc, char *argv[]) {
    ext2_fs_t *fs = _mounted_filesystems[0].fs;
    ext2_bg_t *bg;
    uint32_t bg_num;

    if (argc != 2) {
        printf("usage: show bg <ext2 block group num>\n");
        return;
    }

    bg_num = atoi(argv[1]);
    if ((bg = ext2_get_bg(fs, bg_num)) == NULL) {
        printf("ext2 bg %d read error.\n", bg_num);
        return;
    }

    dump_ext2_bg(bg, bg_num, fs->sb);
}

void do_inode(int argc, char *argv[]) {
    ext2_fs_t *fs = _mounted_filesystems[0].fs;
    ext2_inode_t inode;
    uint32_t inode_num;

    if (argc != 2) {
        printf("usage: show inode <ext2 inode num>\n");
        return;
    }

    inode_num = atoi(argv[1]);
    if (ext2_get_inode(fs, inode_num, &inode) != 0) {
        printf("ext2 inode %d read error.\n", inode_num);
        return;
    }

    dump_ext2_inode(&inode, inode_num);
}

void do_block(int argc, char *argv[]) {
    ext2_fs_t *fs = _mounted_filesystems[0].fs;
    uint32_t block_num;
    int res;

    if (argc != 2) {
        printf("usage: show block <ext2 block num>\n");
        return;
    }

    block_num = atoi(argv[1]);
    res = ext2_read_fs_block(fs, block_num);
    if (res != 0) {
        printf("ext2 block read error. Block num=%d\n", block_num);
        return;
    }

    printf("ext2 block %d:\n", block_num);

    dump_mem(fs->block_buffer, fs->block_size, NO);
}

void do_vars(int argc) {
    ext2_fs_t *fs = _mounted_filesystems[0].fs;

    if (argc != 1) {
        printf("usage: vars\n");
        return;
    }

    printf("Vars:\n");
    printf("  Block Size:       %d (%d)\n", fs->block_size, fs->sb->s_log_block_size);
    printf("  Inodes per Group: %d\n", fs->sb->s_inodes_per_group);
    printf("  Inodes per Block: %d\n", fs->block_size / fs->sb->s_inode_size);
}

void handle_rtc_command(int argc, char *argv[]) {
    register const char *cmd = argv[1];

    if ((argc == 1) || (is_command(cmd, "help", 2) == YES)) {
        printf("rtc accepts the following sub-commands: \n");
        printf("  erase             erase the contents of the rtc's nvram\n");
        printf("  time hh:mm[:ss]   set the time in the rtc\n");
        printf("\n");
    }
    else if (is_command(cmd, "time", 2) == YES) {
        printf("Bob hasn't coded that, tey!!!\n");
    }
    else if (is_command(cmd, "erase", 2) == YES) {
        uint8_t ram[56];
        int res;

        memset(ram, 0xff, 56);
        ram[0] = 0xb0;
        ram[1] = 0xba;

        printf("Erasing rtc nvram...");

        res = ds1307_write_nvram(0, ram, 56);

        printf("\n");

        if (res != 56) {
            printf("ERROR! Erase failed.\n");
        } 
        else {
            printf("Success.\n");
        }
    }
    else {
        printf("'%s' is not a valid sub-command.\n", argv[1]);
    }
}

void handle_set_command(int argc, char *argv[]) {
    register const char *cmd = argv[1];

    if ((argc == 1) || (is_command(cmd, "help", 2) == YES)) {
        printf("set accepts the following sub-commands: \n");
        printf("\n");
    }
}

void handle_show_command(int argc, char *argv[]) {
    register const char *cmd = argv[1];

    if ((argc == 1) || (is_command(cmd, "help", 2) == YES)) {
        printf("show accepts the following sub-commands:\n");
        printf("  bg <num>          show info about Block group <num>\n");
        printf("  block <num>       dump disk block <num>\n");
        printf("  inode <num>       dump inode <num>\n");
        printf("  rtc [nvram]       show rtc related data\n");
        printf("  super             dump the primary superblock\n");
        printf("\n");
    }
    else if (is_command(cmd, "bg", 0) == YES) {
        do_bginfo(argc-1, &argv[1]);
    }
    else if (is_command(cmd, "block", 2) == YES) {
        do_block(argc-1, &argv[1]);
    }
    else if (is_command(cmd, "inode", 2) == YES) {
        do_inode(argc-1, &argv[1]);
    }
    else if (is_command(cmd, "rtc", 2) == YES) {
        do_rtc(argc-1, &argv[1]);
    }
    else if (is_command(cmd, "super", 2) == YES) {
        do_sbinfo(argc-1);
    }
}

void handle_eval_command(int argc, char *argv[]) {
    char exp[MAX_LINE];
    expr_error_t res;
    long val;
    int error_pos;

    exp[0] = EOS;

    for (int i=1; i<argc; i++) {
        strcat(exp, argv[i]);
        strcat(exp, " ");
    }

    printf("Expression string: '%s'\n", exp);
    res = expr_evaluate(exp, &val, &error_pos);
    if (res != EXPR_OK) {
        printf("Failed to parse expression '%s'\n%s\n\n", exp, expr_error_string(res));
    }
    else {
        printf("-> %d (0x%08x)\n", val, val);
    }
}

void handle_command(int argc, char *argv[]) {
    register const char *cmd = argv[0];

    if (is_command(cmd, "quit", 1) == YES) {
        printf("\nBye.\n");
        exit(0);
    }
    else if (is_command(cmd, "eval", 2) == YES) {
        handle_eval_command(argc, argv);
    }
    else if (is_command(cmd, "rtc", 2) == YES) {
        handle_rtc_command(argc, argv);
    }
    else if (is_command(cmd, "set", 2) == YES) {
        handle_set_command(argc, argv);
    }
    else if (is_command(cmd, "show", 2) == YES) {
        handle_show_command(argc, argv);
    }
    else if (is_command(cmd, "vars", 2) == YES) {
        do_vars(argc);
    }
    else if (is_command(cmd, "help", 2) == YES) {
        printf("Commands are:\n");
        printf("  show [ bg <num> | inode <num> | super | block <num> | ? ]\n");
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