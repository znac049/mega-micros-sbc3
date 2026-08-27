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
#include <extras.h>
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

int ext2_opendir(vfile_t *dir, const char *name) {
    char name_copy[EXT2_MAX_PATH_LEN];
    char *s = name_copy;
    char *dirv[EXT2_MAX_DIR_DEPTH];
    int dirc = 0;
    int dir_inode_num = EXT2_ROOT_INO;
    ext2_dirp_t *dirp;
    vmp_t *mp = dir->mp;

    printf("ext2_opendir: '%s'\n", name);

    if (mp == NULL) {
        kprintf("NULL vmp_t in vfile_t\n");
        return NOT_OK;
    }

    dirp = &dir->fs.e2dir;
    dirp->offset = 0;
    dirp->mp = mp;

    strcpy(name_copy, name);
    
    if (*s == '/') {
        s++;
    }

    if (*s) {
        dirc = split_str(s, '/', dirv, EXT2_MAX_DIR_DEPTH);

        kprintf("The path splits into %d parts:\n", dirc);
        for (int i=0; i<dirc; i++) {
            kprintf("  %s\n", dirv[i]);
        }
    }

    // do stuff
    if (dirc) {
        for (int i=0; (i<dirc) && (dir_inode_num != -1); i++) {
            dir_inode_num = find_dir_inode_in(&mp->fs_data.e2fs, dir_inode_num, dirv[i]);
            kprintf("Directory inode # is %d\n", dir_inode_num);
        }
    }

    kprintf("The final dir inode # is %d\n", dir_inode_num);
    ext2_init_block_follower(&mp->fs_data.e2fs, dir_inode_num, &dirp->bf);

    return OK;
}

int ext2_closedir(ext2_dirp_t *dirp) {
    if (dirp == NULL) {
        errno = EBADF;
        return NOT_OK;
    }

    dirp->mp = NULL;

    return OK;
}

ext2_dirent_t *ext2_readdir(ext2_dirp_t *dirp) {
    uint8_t *buf = (uint8_t *)&dirp->mp->block_buff;

    if (dirp == NULL) {
        errno = EBADF;
        return NULL;
    }

    // do stuff
    if (dirp->offset == 0) {
        uint32_t block_num = ext2_get_next_block_num(&dirp->bf);

        if (block_num == 0) {
            return NULL;
        }

        printf("Grab dir block %d\n", block_num);
        if (ext2_read_fs_block(&dirp->mp->fs_data.e2fs, block_num) != 0) {
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

    ext2_reset_block_follower(&dirp->bf);
    dirp->offset = 0;
}

// vfile_t *ext2_locate(vmp_t *mp, vfile_t *dir, const char *pathname) {
//     ext2_dirp_t *e2_dir;

//     kprintf("ext2_locate(..., '%s')\n", pathname);

//     if ((mp == NULL) || (dir == NULL)) {
//         kprintf("mp and/or dir set to NULL\n");
//         return NULL;
//     }

//     e2_dir = ext2_opendir(mp, dir, pathname);

//     if (e2_dir == NULL) {
//         kprintf("Failed to open ext2 dir '%s'\n", pathname);
//         return NULL;
//     }

//     return dir;
// }

#endif
