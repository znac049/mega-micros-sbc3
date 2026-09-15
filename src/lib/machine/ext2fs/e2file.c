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
#include <unistd.h>
#include <errno.h>

#if defined(BAREMETAL)

int ext2_open(vfile_t *file, const char *name, int flags, vfile_t *cwd) {
    ext2_file_t *filep = &file->private.data.ext2_file_inf;
    int file_inode_num = EXT2_ROOT_INO;
    char *dirv[EXT2_MAX_DIR_DEPTH];
    int dirc;
    ext2_fs_t *fs;
    ext2_inode_t inode;
    uint8_t file_type;

    // kprintf("ext2_do_open: '%s'\n", name);

    // Creating/writing is not currently coded, so reject any flags
    // that indicate creating/writing is required
    if (flags & (O_WRONLY | O_CREAT | O_TRUNC | O_APPEND)) {
        // Not currently supported
        kprintf("ext2_do_open: '%s', %04x - writing not supported yet!", name, flags);

        return NOT_OK;
    }

    if (file->mp == NULL) {
        kprintf("ext2_do_open: NULL vmp_t in vfile_t\n");
        return NOT_OK;
    }

    fs = &file->mp->private.data.ext2_fs_inf;

    if (strcmp(name, "") == 0) {
        if (cwd->open == YES) {
            // use the current directory
            // kprintf("ext2_do_open: using the current directory\n");

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

    // kprintf("ext2_do_open: '%s' splits into %d parts:\n", name, dirc);
    // for (int i=0; i<dirc; i++) {
    //     kprintf("  %s\n", dirv[i]);
    // }

    for (int i=0; i<dirc; i++) {
        int parent_inode_num = file_inode_num;

        if ((file_inode_num = e2_search(file->mp, file_inode_num, dirv[i], &file_type)) == 0) {
            kprintf("ext2_do_open: couldn't find '%s' in directory with inode %d\n", dirv[i], parent_inode_num);
            return NOT_OK;
        }

        if (ext2_get_inode(file->mp, file_inode_num, &inode) == NOT_OK) {
            kprintf("ext2_do_open: failed to read inode %d\n", file_inode_num);
            return NOT_OK;
        }

        if (i < dirc-1) {
            // It must be a directory
            if (file_type != VFS_FT_DIR) {
                kprintf("Found '%s' but it's not a directory!\n", dirv[i]);
                return NOT_OK;
            }
        }
    }

    // kprintf("ext_open: final inode is %d\n", file_inode_num);

    ext2_init_block_follower(&filep->bf, file->mp, file_inode_num);

    file->size = inode.i_size;
    file->mode = inode.i_mode;
    file->file_type = file_type;

    // kprintf("ext2_open: '%s' opened ok. mode=%04x, size=%d\n", name, file->mode, file->size);

    return OK;
}

int ext2_read(vfile_t *file, char *buff, size_t count) {
    ext2_file_t *filep = &file->private.data.ext2_file_inf;
    uint32_t block_num;

    // kprintf("ext2_read: request to read %d bytes into buffer @ 0x%08x\n", count, buff);
    // kprintf("ext2_read: file size=%d, pos=%d\n", file->size, file->position);

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
        // kprintf("ext2_read: it's a hole\n");
        memset(buff, 0, BLOCK_DEVICE_BLOCK_SIZE);
    }
    else {
        if (ext2_read_block(file->mp, block_num, (uint8_t *)buff) == NOT_OK) {
            kprintf("ext2_read: failed to read block %d\n", block_num);
            return NOT_OK;
        }
    }

    return BLOCK_DEVICE_BLOCK_SIZE;
}

int ext2_write(vfile_t *file, const char *buff, size_t count) {
    return NOT_OK;
}

int ext2_close(vfile_t *file) {
    return OK;
}

static int seek_to(vfile_t *file, off_t offset) {
    uint32_t block = 0;
    ext2_file_t *filep = &file->private.data.ext2_file_inf;
    uint32_t block_num = offset / BLOCK_DEVICE_BLOCK_SIZE;

    kprintf("seek_to: reset block follower\n");
    kprintf("seek_to: file pos=%d, size=%d. abs offset=%d\n", 
            file->position, file->size, offset);
    kprintf("seek_to: data resides in relative block %d\n", block_num);

    ext2_reset_block_follower(&filep->bf);
    file->position = 0;

    dump_ext2_inode(&filep->bf.inode, filep->bf.inode_num);

    // block = ext2_get_next_block_num(&filep->bf);
    // kprintf("seek_to: first block=%d\n", block);

    for (uint32_t i=0; i<block_num; i++) {
        block = ext2_get_next_block_num(&filep->bf);
        file->position += BLOCK_DEVICE_BLOCK_SIZE;

        kprintf("seek_to: next block=%d, pos=%d\n", block, file->position);
    }

    if (block == 0) {
        kprintf("seek_to: got a hole.\n");
        memset(file->buffer, 0, BLOCK_DEVICE_BLOCK_SIZE);
    }
    else {
        if (ext2_read_block(file->mp, block, (uint8_t *)file->buffer) == NOT_OK) {
            kprintf("seek_to: failed to read block %d\n", block_num);
            return NOT_OK;
        }
    }

    file->index = offset % BLOCK_DEVICE_BLOCK_SIZE;
    file->count = BLOCK_DEVICE_BLOCK_SIZE;
    file->position = offset;

    if (file->position > file->size) {
        kprintf("seek_to: PAST EOF\n");
        
        file->position = file->size;
        file->index = file->size % BLOCK_DEVICE_BLOCK_SIZE;
    }

    kprintf("seek_to: final file pos=%d, size=%d, index=%d, offset=%d\n", file->position, file->size, file->index, offset);

    return file->position;
}

int ext2_seek(vfile_t *file, off_t offset, int whence) {
    off_t abs_offset = offset;

    switch (whence) {
        case SEEK_SET:
            break;

        case SEEK_CUR:
        abs_offset = file->position + offset;
            break;

        case SEEK_END:
            abs_offset = file->size + offset;
            break;
    }

    kprintf("ext2_seek: abs_offset=%d\n", abs_offset);

    if (abs_offset == file->position) {
        kprintf("ext2_seek: we're already at the right place (%d):-)\n", file->position);
    }
    else {
        // Check to see if it's in the current block
        off_t start_off = file->position - file->index;
        off_t end_off = start_off + BLOCK_DEVICE_BLOCK_SIZE;

        if ((abs_offset >= start_off) && (abs_offset < end_off)) {
            // Yes, it's in the current block, so...
            off_t diff = abs_offset - file->position;

            kprintf("ext2_seek: current block, delta=%d\n", diff);
            file->position += diff;
            file->index += diff;
        }
        else {
            kprintf("ext2_seek: seek absolute %d...\n", abs_offset);
            seek_to(file, abs_offset);
        }
    }

    kprintf("ext2_seek: returning %d\n", file->position);
    
    return file->position;
}

#endif