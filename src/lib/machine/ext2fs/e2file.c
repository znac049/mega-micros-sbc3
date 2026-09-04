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

#include <stddef.h>
#include <ctype.h>
#include <machine.h>
#include <ext2.h>
#include <string.h>
#include <extras.h>

#if defined(BAREMETAL)

int ext2_open(vfile_t *file, const char *name, vfile_t *cwd) {
    ext2_file_t *filep = &file->private.data.ext2_file_inf;
    int file_inode_num = EXT2_ROOT_INO;
    char *dirv[EXT2_MAX_DIR_DEPTH];
    int dirc;
    ext2_fs_t *fs;

    // kprintf("ext2_open: '%s'\n", name);

    if (file->mp == NULL) {
        kprintf("ext2_open: NULL vmp_t in vfile_t\n");
        return NOT_OK;
    }

    fs = &file->mp->private.data.ext2_fs_inf;

    if (strcmp(name, "") == 0) {
        if (cwd->open == YES) {
            // use the current directory
            // kprintf("ext2_open: using the current directory\n");

            memcpy(file, cwd, sizeof(vfile_t));
            ext2_init_block_follower(&filep->bf, file->mp, file_inode_num);

            dump_ext2_fs(fs);
            
            return OK;
        }
        else {
            // We don't currently have a current dir. Assume root of current mount
            strcpy(file->path, "");
            file->open = YES;
            ext2_init_block_follower(&filep->bf, file->mp, file_inode_num);

            filep->offset = 0;

            return OK;
        }
    }
    
    dirc = split_str(name, '/', dirv, EXT2_MAX_DIR_DEPTH);

    // kprintf("ext2_open: '%s' splits into %d parts:\n", name, dirc);
    for (int i=0; i<dirc; i++) {
        kprintf("  %s\n", dirv[i]);
    }

    for (int i=0; i<dirc; i++) {
        int parent_inode_num = file_inode_num;

        if ((file_inode_num = e2_search(file->mp, file_inode_num, dirv[i])) == 0) {
            kprintf("ext2_open: couldn't find '%s' in directory with inode %d\n", dirv[i], parent_inode_num);
            return NOT_OK;
        }
    }

    kprintf("ext_open: final inode is %d\n");

    ext2_init_block_follower(&filep->bf, file->mp, file_inode_num);

    return OK;
}

int ext2_read(vfile_t *file, char *buff, size_t count) {
    ext2_file_t *filep = &file->private.data.ext2_file_inf;
    uint32_t block_num;

    kprintf("ext2_read: request to read %d bytes into buffer @ 0x%08x\n", count, buff);

    if (file->mp == NULL) {
        kprintf("ext2_read: NULL vmp_t in vfile_t\n");
        return NOT_OK;
    }

    if (count < BLOCK_DEVICE_BLOCK_SIZE) {
        kprintf("ext2_read: target buffer is not big enough (%d)\n", count);
        return NOT_OK;
    }

    block_num = ext2_get_next_block_num(&filep->bf);
    if (block_num == 0) {
        kprintf("ext2_read: no more blocks to read\n");
        return NOT_OK;
    }

    if (ext2_read_block(file->mp, block_num, (uint8_t *)buff) == NOT_OK) {
        kprintf("ext2_read: failed to read block %d\n", block_num);
        return NOT_OK;
    }

    return BLOCK_DEVICE_BLOCK_SIZE;
}

int ext2_write(vfile_t *file, const char *buff, size_t count) {
    return NOT_OK;
}

int ext2_close(vfile_t *file) {
    return NOT_OK;
}

#endif