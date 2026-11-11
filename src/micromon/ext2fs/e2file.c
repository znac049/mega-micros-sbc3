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
#include <string.h>
#include <extras.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <fcntl.h>

#include "micromon.h"

/*
 * Open a file or directory on an ext2 filesystem.
 *
 * name is relative to the root of the mount and already normalised by
 * the VFS layer: no leading slash, no "." or ".." components, no
 * repeated slashes. "" means the root directory of the mount.
 */
int ext2_open(vfile_t *file, const char *name, int flags, vfile_t *cwd) {
    ext2_file_t *filep = &file->private.data.ext2_file_inf;
    char path[PATH_MAX];
    char *dirv[EXT2_MAX_DIR_DEPTH];
    int dirc = 0;
    uint32_t inode_num = EXT2_ROOT_INO;
    ext2_inode_t inode;

    (void)cwd;      // The VFS always gives us a path from the mount's root

    // Creating/writing is not currently coded, so reject any flags
    // that indicate creating/writing is required
    if (flags & (O_WRONLY | O_CREAT | O_TRUNC | O_APPEND)) {
        // Not currently supported
        kprintf("ext2_open: '%s', %04x - writing not supported yet!\n", name, flags);
        errno = EROFS;

        return NOT_OK;
    }

    if (file->mp == NULL) {
        kprintf("ext2_open: NULL vmp_t in vfile_t\n");
        errno = EBADF;

        return NOT_OK;
    }

    if (strlen(name) >= sizeof(path)) {
        errno = ENAMETOOLONG;
        return NOT_OK;
    }

    // split_str() writes into the string it splits, so work on a copy
    strcpy(path, name);
    if (path[0] != EOS) {
        dirc = split_str(path, '/', dirv, EXT2_MAX_DIR_DEPTH);
    }

    // Start at the root directory and walk down the path
    if (ext2_get_inode(file->mp, inode_num, &inode) == NOT_OK) {
        errno = EIO;
        return NOT_OK;
    }

    for (int i=0; i<dirc; i++) {
        uint32_t parent_inode_num = inode_num;

        // We can only look inside directories
        if (!S_ISDIR(inode.i_mode)) {
            errno = ENOTDIR;
            return NOT_OK;
        }

        inode_num = e2_search(file->mp, parent_inode_num, dirv[i], NULL);
        if (inode_num == 0) {
            // kprintf("ext2_open: couldn't find '%s' in directory with inode %d\n", dirv[i], parent_inode_num);
            errno = ENOENT;
            return NOT_OK;
        }

        if (ext2_get_inode(file->mp, inode_num, &inode) == NOT_OK) {
            kprintf("ext2_open: failed to read inode %d\n", inode_num);
            errno = EIO;
            return NOT_OK;
        }
    }

    if ((flags & O_DIRECTORY) && !S_ISDIR(inode.i_mode)) {
        errno = ENOTDIR;
        return NOT_OK;
    }

    if (ext2_init_block_follower(&filep->bf, file->mp, inode_num) == NOT_OK) {
        errno = EIO;
        return NOT_OK;
    }

    filep->mp = file->mp;
    filep->offset = 0;

    file->size = inode.i_size;
    file->mode = inode.i_mode;

    // Take the type from the inode - the directory entry's file type
    // byte is only present if the fs has the 'filetype' feature
    if (S_ISDIR(inode.i_mode)) {
        file->file_type = VFS_FT_DIR;
    }
    else if (S_ISREG(inode.i_mode)) {
        file->file_type = VFS_FT_REG;
    }
    else if (S_ISLNK(inode.i_mode)) {
        file->file_type = VFS_FT_SYMLINK;
    }
    else {
        file->file_type = VFS_FT_UNKNOWN;
    }

    // kprintf("ext2_open: '%s' opened ok. inode=%d, mode=%04x, size=%d\n", name, inode_num, file->mode, file->size);

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
    (void)file;
    (void)buff[0];
    (void)count;
    
    return NOT_OK;
}

int ext2_close(vfile_t *file) {
    (void)file;

    return OK;
}

/*
 * Load the block containing offset into the file's buffer and leave the
 * block follower pointing at the block after it, so the next ext2_read()
 * carries on from there.
 */
static int seek_to(vfile_t *file, off_t offset) {
    uint32_t block = 0;
    ext2_file_t *filep = &file->private.data.ext2_file_inf;
    uint32_t block_num = offset / BLOCK_DEVICE_BLOCK_SIZE;

    // kprintf("seek_to: file pos=%d, size=%d. abs offset=%d (relative block %d)\n",
    //         file->position, file->size, offset, block_num);

    ext2_reset_block_follower(&filep->bf);

    // Step through blocks 0..block_num - 'block' ends up as block_num's
    for (uint32_t i=0; i<=block_num; i++) {
        block = ext2_get_next_block_num(&filep->bf);
    }

    if (block == 0) {
        // A hole (or past the end of the file)
        memset(file->buffer, 0, BLOCK_DEVICE_BLOCK_SIZE);
    }
    else {
        if (ext2_read_block(file->mp, block, (uint8_t *)file->buffer) == NOT_OK) {
            kprintf("seek_to: failed to read block %d\n", block);
            file->index = file->count = 0;
            errno = EIO;
            return NOT_OK;
        }
    }

    file->index = offset % BLOCK_DEVICE_BLOCK_SIZE;
    file->count = BLOCK_DEVICE_BLOCK_SIZE;
    file->position = offset;

    if (file->position > (off_t)file->size) {
        file->position = file->size;
        file->index = file->size % BLOCK_DEVICE_BLOCK_SIZE;
    }

    return file->position;
}

int ext2_seek(vfile_t *file, off_t offset, int whence) {
    off_t abs_offset;

    switch (whence) {
        case SEEK_SET:
            abs_offset = offset;
            break;

        case SEEK_CUR:
            abs_offset = file->position + offset;
            break;

        case SEEK_END:
            abs_offset = file->size + offset;
            break;

        default:
            errno = EINVAL;
            return NOT_OK;
    }

    if (abs_offset < 0) {
        errno = EINVAL;
        return NOT_OK;
    }

    if (abs_offset != file->position) {
        // Is it in the block we already have in the buffer?
        off_t start_off = file->position - file->index;
        off_t end_off = start_off + file->count;

        if ((file->count > 0) && (abs_offset >= start_off) && (abs_offset < end_off)) {
            off_t diff = abs_offset - file->position;

            file->position += diff;
            file->index += diff;
        }
        else if (seek_to(file, abs_offset) == NOT_OK) {
            return NOT_OK;
        }
    }

    return file->position;
}
