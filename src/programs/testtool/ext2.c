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

void handle_sbinfo_subcommand(int argc) {
    ext2_fs_t *fs = _mounted_filesystems[0].fs;

    if (argc != 1) {
        printf("usage: show super\n");
        return;
    }

    dump_ext2_sb(fs->sb);
}

void handle_bginfo_subcommand(int argc, char *argv[]) {
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

void handle_inode_subcommand(int argc, char *argv[]) {
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

void handle_block_subcommand(int argc, char *argv[]) {
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

