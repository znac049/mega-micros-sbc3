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
#include <machine.h>
#include <disk.h>
#include <ext2.h>
#include <nonstd.h>

#include "dir.h"

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

void list_dir(const char *dir_name) {
    ext2_fs_t *fs = _mounted_filesystems[0].fs;
    ext2_dir_t *dirp = ext2_opendir(fs, dir_name);
    ext2_dirent_t *ent;

    printf("Directory '%s':\n", dir_name);

    if (dirp == NULL) {
        return;
    }

    ent = ext2_readdir(dirp);
    while (ent != NULL) {
        char ch = '?';

        switch (ent->file_type) {
            case EXT2_FT_REG_FILE: ch = 'f'; break;
            case EXT2_FT_DIR: ch = 'd'; break;
            case EXT2_FT_CHRDEV: ch = 'c'; break;
            case EXT2_FT_BLKDEV: ch = 'b'; break;
            case EXT2_FT_FIFO: ch = 'F'; break;
            case EXT2_FT_SOCK: ch = 's'; break;
            case EXT2_FT_SYMLINK: ch = 'l'; break;
        }

        printf("%c %8d ", ch, ent->inode);

        for (int i=0; i<ent->name_len; i++) {
            printf("%c", ent->name[i]);
        }
        printf("\n");

        ent = ext2_readdir(dirp);
    }

    ext2_closedir(dirp);
}

int main(void) {
    if (!cf_present()) {
        printf("No CF drive(s) present.\n");
        exit(1);
    }

    if (_num_mounted_filesystems) {
        list_dir("");
        list_dir("etc");
    }

    return 0;
}

