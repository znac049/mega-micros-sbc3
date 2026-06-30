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
        printf("usage: sbinfo\n");
        return;
    }

    dump_ext2_sb(fs->sb);
}

void do_bginfo(int argc, char *argv[]) {
    ext2_fs_t *fs = _mounted_filesystems[0].fs;
    ext2_bg_t *bg;
    uint32_t bg_num;

    if (argc != 2) {
        printf("usage: bginfo <ext2 block group num>\n");
        return;
    }

    bg_num = atoi(argv[1]);
    if ((bg = ext2_get_bg(fs, bg_num)) == NULL) {
        printf("ext2 bg %d read error.\n", bg_num);
        return;
    }

    dump_ext2_bg(bg, bg_num, fs->sb);
}

void do_inodeinfo(int argc, char *argv[]) {
    ext2_fs_t *fs = _mounted_filesystems[0].fs;
    ext2_inode_t inode;
    uint32_t inode_num;

    if (argc != 2) {
        printf("usage: inode <ext2 inode num>\n");
        return;
    }

    inode_num = atoi(argv[1]);
    if (ext2_get_inode(fs, inode_num, &inode) != 0) {
        printf("ext2 inode %d read error.\n", inode_num);
        return;
    }

    dump_ext2_inode(&inode, inode_num);
}

void do_dump(int argc, char *argv[]) {
    ext2_fs_t *fs = _mounted_filesystems[0].fs;
    uint32_t block_num;
    int res;

    if (argc != 2) {
        printf("usage: dump <ext2 block num>\n");
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

void handle_command(int argc, char *argv[]) {
    if (strcasecmp(argv[0], "quit") == 0) {
        printf("\nBye.\n");
        exit(0);
    }
    else if (strcasecmp(argv[0], "bginfo") == 0) {
        do_bginfo(argc, argv);
    }
    else if (strcasecmp(argv[0], "dump") == 0) {
        do_dump(argc, argv);
    }
    else if (strcasecmp(argv[0], "inode") == 0) {
        do_inodeinfo(argc, argv);
    }
    else if (strcasecmp(argv[0], "sbinfo") == 0) {
        do_sbinfo(argc);
    }
    else if (strcasecmp(argv[0], "vars") == 0) {
        do_vars(argc);
    }
    else if (strcasecmp(argv[0], "help") == 0) {
        printf("Commands are:\n");
        printf("  bginfo <ext2 block group number>\n");
        printf("  dump <ext2 block number>\n");
        printf("  inode <ext2 inode num>\n");
        printf("  sbinfo\n");
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
                // printf("CMD: '%s'\n", argv[0]);
                handle_command(argc, argv);
            }
        }
    }

    printf("\n\nBye.\n");

    return 0;
}