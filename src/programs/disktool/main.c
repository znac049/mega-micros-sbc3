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

#include "disktool.h"


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

void handle_show_command(int argc, char *argv[]) {
    register const char *cmd = argv[1];

    if (argc == 1) {
        printf("show accepts the following arguments: \n");
        printf("  bg <num>       show info about Block group <num>\n");
        printf("  block <num>    dump disk block <num>\n");
        printf("  inode <num>    dump inode <num>\n");
        printf("  super          dump the primary superblock\n");
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
    else if (is_command(cmd, "super", 2) == YES) {
        do_sbinfo(argc-1);
    }
}

void handle_command(int argc, char *argv[]) {
    register const char *cmd = argv[0];

    if (is_command(cmd, "quit", 1) == YES) {
        printf("\nBye.\n");
        exit(0);
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

    printf("Disktool v1.0.\n");
    
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