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
    kprintf("\nDirectory entry:\n");
    kprintf(" inode #:    %d\n", ent->inode);
    kprintf(" record len: %d\n", ent->rec_len);
    kprintf(" name_len:   %d\n", ent->name_len);
    kprintf(" file type:  0x%02x\n", ent->file_type);
    kprintf(" name:       ");
    for (int i=0; i<ent->name_len; i++) {
        kprintf("%c", ent->name[i]);
    }
    kprintf("\n");
}

#if 0
static int names_match(const char *target, const uint8_t *possible, uint8_t plen) {
    int tlen = strlen(target);

    kprintf("do '%s' and '%s' match? ", target, possible);

    if (tlen != plen) {
        kprintf("NO :-(\n");
        return NO;
    }

    for (int i=0; i<plen; i++) {
        if (target[i] != possible[i]) {
            kprintf("NO :-(\n");
            return NO;
        }
    }

    kprintf("YES!\n");
    return YES;
}
#endif

#if 0
uint32_t ext2_find_item_inode_in(vmp_t *mp, uint32_t parent_inode_num, const char *item_name, bool_t is_dir) {
    ext2_inode_t parent;
    int res = ext2_get_inode(mp, parent_inode_num, &parent);
    ext2_block_follower_t bf;
    ext2_dirent_t *dirent;

    kprintf("Looking for a %s called '%s' in directory at inode # %d\n", 
        is_dir?"directory":"file",
        item_name, parent_inode_num);

    if (res == NOT_OK) {
        kprintf("ext2_get_inode(%d) failed\n", parent_inode_num);
        return NOT_OK;
    }

    // We have the parent directory inode, look through it's ext2_dirent_t entries
    ext2_init_block_follower(fs, parent_inode_num, &bf);
    for (uint32_t block_num=ext2_get_next_block_num(&bf); block_num != 0; block_num=ext2_get_next_block_num(&bf)) {
        ext2_dirent_t ent;
        uint32_t offset = 0;

        kprintf("Read block %d\n", block_num);

        ext2_read_fs_block(fs, block_num);
        while (offset < BLOCK_DEVICE_BLOCK_SIZE) {
            dirent = (ext2_dirent_t *)&fs->block_buffer[offset];
            ext2_sanitize_dirent(dirent, &ent);

            kprintf("  name->'%s'\n", ent.name);

            if ( ((is_dir == YES) && (ent.file_type == EXT2_FT_DIR)) ||
                 ((is_dir == NO) && (ent.file_type != EXT2_FT_DIR)) ) {
                if (names_match(item_name, ent.name, ent.name_len)) {
                    kprintf("BINGO! in=%d, rl=%d, nl=%d\n", ent.inode, ent.rec_len, ent.name_len);
                    return ent.inode;
                }
            }

            offset += ent.rec_len;
        }

    }

    // for (int block_index=0; block_index < EXT2_SNGL_IND; block_index++) {
    //     ext2_dirent_t ent;
    //     uint32_t offset = 0;

    //     if (parent.i_block[block_index] == 0) {
    //         kprintf("Not found!\n");
    //         return NOT_OK;
    //     }

    //     kprintf("Read block %d\n", parent.i_block[block_index]);

    //     ext2_read_fs_block(fs, parent.i_block[block_index]);
    //     while (offset < BLOCK_DEVICE_BLOCK_SIZE) {
    //         ext2_sanitize_dirent((ext2_dirent_t *)&fs->block_buffer[offset], &ent);


    //         if ( ((is_dir == YES) && (ent.file_type == EXT2_FT_DIR)) ||
    //              ((is_dir == NO) && (ent.file_type != EXT2_FT_DIR)) ) {
    //             if (names_match(item_name, ent.name, ent.name_len)) {
    //                 kprintf("BINGO! in=%d, rl=%d, nl=%d\n", ent.inode, ent.rec_len, ent.name_len);
    //                 return ent.inode;
    //             }
    //         }

    //         offset += ent.rec_len;
    //     }
    // }

    return parent_inode_num;
}
#endif

#if 0
static int ext2_find_path(vfile_t *dir, const char *name) {
    char name_copy[EXT2_MAX_PATH_LEN];
    char *s = name_copy;
    char *dirv[EXT2_MAX_DIR_DEPTH];
    int dirc = 0;
    int dir_inode_num = EXT2_ROOT_INO;
    ext2_dirp_t *dirp;
    vmp_t *mp = dir->mp;

    kprintf("ext2_find_path: '%s'\n", name);

    if (mp == NULL) {
        kprintf("NULL vmp_t in vfile_t\n");
        return NOT_OK;
    }

    dirp = &dir->fs.private;
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
            dir_inode_num = ext2_find_item_inode_in(&mp->fs_data.e2fs, dir_inode_num, dirv[i], YES);
            kprintf("Directory inode # is %d\n", dir_inode_num);
        }
    }

    kprintf("The final dir inode # is %d\n", dir_inode_num);

    ext2_init_block_follower(&mp->fs_data.e2fs, dir_inode_num, &dirp->bf);

    return OK;
}
#endif

int ext2_closedir(ext2_file_t *dirp) {
    if (dirp == NULL) {
        errno = EBADF;
        return NOT_OK;
    }

    dirp->mp = NULL;

    return OK;
}

ext2_dirent_t *ext2_readdir(ext2_file_t *file) {
    uint8_t *buf = (uint8_t *)&file->mp->block_buffer;

    if (file == NULL) {
        errno = EBADF;
        return NULL;
    }

    // do stuff
    if (file->offset == 0) {
        uint32_t block_num = ext2_get_next_block_num(&file->bf);

        if (block_num == 0) {
            return NULL;
        }

        kprintf("Grab dir block %d\n", block_num);
        if (ext2_read_fs_block(file->mp, block_num, YES) != 0) {
            kprintf("Failed to read block %d\n", block_num);
            return NULL;
        }
    }

    kprintf("dirent offset=%d\n", file->offset);
    ext2_sanitize_dirent((ext2_dirent_t *)&buf[file->offset], &file->dirent);
    // dump_dirent((ext2_dirent_t *)&buf[file->offset]);

    // update the offset ready for the next call
    file->offset += file->dirent.rec_len;
    if (file->offset >= BLOCK_DEVICE_BLOCK_SIZE) {
        file->offset = 0;
    }

    return &file->dirent;
}

void ext2_rewinddir(ext2_file_t *file) {
    if (file == NULL) {
        errno = EBADF;
        return;
    }

    ext2_reset_block_follower(&file->bf);
    file->offset = 0;
}

#endif
