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
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <nonstd.h>
#include <machine.h>
#include <ext2.h>

#if defined(BAREMETAL)

void dump_dirent (ext2_dirent_t *ent) {
    printf("\nDirectory entry:\n");
    printf(" inode #:    %d\n", ent->inode);
    printf(" record len: %d\n", ent->rec_len);
    printf(" name_len:   %d\n", ent->name_len);
    printf(" file type:  0x%02x\n", ent->file_type);
    printf(" name:       ");
    for (int i=0; i<ent->name_len; i++) {
        printf("%c", ent->name[i]);
    }
    printf("\n");
}

static int names_match(const char *target, const uint8_t *possible, uint8_t plen) {
    int tlen = strlen(target);

    if (tlen != plen) {
        return 0;
    }

    for (int i=0; i<plen; i++) {
        if (target[i] != possible[i]) {
            return 0;
        }
    }

    return 1;
}

static uint32_t find_dir_inode_in(ext2_fs_t *fs, uint32_t parent_inode_num, const char *dir_name) {
    ext2_inode_t parent;
    int res = ext2_get_inode(fs, parent_inode_num, &parent);

    printf("Looking for a subdirectory called '%s' in directory at inode # %d\n", dir_name, parent_inode_num);

    if (res != 0) {
        return -1;
    }

    // We have the parent directory inode, look through the directory
    for (int block_index=0; block_index < EXT2_SNGL_IND; block_index++) {
        ext2_dirent_t ent;
        uint32_t offset = 0;

        if (parent.i_block[block_index] == 0) {
            printf("Not found!\n");
            return -1;
        }

        printf("Read block %d\n", parent.i_block[block_index]);

        ext2_read_fs_block(fs, parent.i_block[block_index]);
        while (offset < BLOCK_DEVICE_BLOCK_SIZE) {
            ext2_sanitize_dirent((ext2_dirent_t *)&fs->block_buffer[offset], &ent);

            if (ent.file_type == EXT2_FT_DIR) {
                if (names_match(dir_name, ent.name, ent.name_len)) {
                    printf("BINGO! in=%d, rl=%d, nl=%d\n", ent.inode, ent.rec_len, ent.name_len);
                    return ent.inode;
                }
            }

            offset += ent.rec_len;
        }
    }

    return parent_inode_num;
}

ext2_dirp_t *ext2_opendir(ext2_fs_t *fs, const char *name) {
    ext2_dirp_t *dirp;
    char name_copy[EXT2_MAX_PATH_LEN];
    char *s = name_copy;
    char *dirv[EXT2_MAX_DIR_DEPTH];
    int dirc = 0;
    int dir_inode_num = EXT2_ROOT_INO;

    printf("ext2_opendir: '%s'\n", name);
    strcpy(name_copy, name);
    
    if (*s == '/') {
        s++;
    }

    if (*s) {
        dirc = split_str(s, '/', dirv, EXT2_MAX_DIR_DEPTH);

        printf("The path splits into %d parts:\n", dirc);
        for (int i=0; i<dirc; i++) {
            printf("  %s\n", dirv[i]);
        }
    }

    dirp = malloc(sizeof(ext2_dirp_t));

    if (dirp == NULL) {
        errno = ENOMEM;
        return NULL;
    }

    dirp->bf = malloc(sizeof(ext2_block_follower_t));
    dirp->buffer = malloc(BLOCK_DEVICE_BLOCK_SIZE);
    dirp->fs = fs;
    dirp->offset = 0;

    if ((dirp->buffer == NULL) || (dirp->bf == NULL)) {
        free(dirp->bf);
        free(dirp->buffer);
        free(dirp);
        errno = ENOMEM;
        return NULL;
    }

    // do stuff
    if (dirc) {
        for (int i=0; (i<dirc) && (dir_inode_num != -1); i++) {
            dir_inode_num = find_dir_inode_in(fs, dir_inode_num, dirv[i]);
            printf("Directory inode # is %d\n", dir_inode_num);
        }
    }

    printf("The final dir inode # is %d\n", dir_inode_num);
    ext2_init_block_follower(fs, dir_inode_num, dirp->bf);

    return dirp;
}

int ext2_closedir(ext2_dirp_t *dirp) {
    if (dirp == NULL) {
        errno = EBADF;
        return -1;
    }

    free(dirp->bf);
    free(dirp->buffer);
    free(dirp);

    return 0;
}

ext2_dirent_t *ext2_readdir(ext2_dirp_t *dirp) {
    uint8_t *buf = dirp->fs->block_buffer;

    if (dirp == NULL) {
        errno = EBADF;
        return NULL;
    }

    // do stuff
    if (dirp->offset == 0) {
        uint32_t block_num = ext2_get_next_block_num(dirp->bf);

        if (block_num == 0) {
            return NULL;
        }

        printf("Grab dir block %d\n", block_num);
        if (ext2_read_fs_block(dirp->fs, block_num) != 0) {
            printf("Failed to read block %d\n", block_num);
            return NULL;
        }
    }

    printf("dirent offset=%d\n", dirp->offset);
    ext2_sanitize_dirent((ext2_dirent_t *)&buf[dirp->offset], &dirp->dirent);
    // dump_dirent((ext2_dirent_t *)&buf[dirp->offset]);

    // update the offset ready for the next call
    dirp->offset += dirp->dirent.rec_len;
    if (dirp->offset >= BLOCK_DEVICE_BLOCK_SIZE) {
        dirp->offset = 0;
    }

    return &dirp->dirent;
}

void ext2_rewinddir(ext2_dirp_t *dirp) {
    if (dirp == NULL) {
        errno = EBADF;
        return;
    }

    ext2_reset_block_follower(dirp->bf);
    dirp->offset = 0;
}

#endif
