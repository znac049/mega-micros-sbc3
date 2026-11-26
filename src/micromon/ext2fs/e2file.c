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
#include <sys/stat.h>

#include "micromon.h"

/*
 * Make a new, empty regular file called 'name' in directory parent_inode.
 * Returns its inode number in *inode_num and its inode in *inode.
 */
static int create_file(vmp_t *mp, uint32_t parent_inode, const char *name, uint32_t mode,
                       uint32_t *inode_num, ext2_inode_t *inode) {
    size_t len = strlen(name);
    uint32_t now = ext2_now();

    if ((len == 0) || (len > EXT2_MAX_LEN) || (strcmp(name, ".") == 0) || (strcmp(name, "..") == 0)) {
        errno = EINVAL;
        return NOT_OK;
    }

    if (ext2_alloc_inode(mp, parent_inode, NO, inode_num) != OK) {
        return NOT_OK;
    }

    memset(inode, 0, sizeof(*inode));
    inode->i_mode = S_IFREG | (mode & 07777);
    inode->i_links_count = 1;
    inode->i_atime = inode->i_ctime = inode->i_mtime = now;

    /*
     * Write the inode before the directory entry that points to it: if
     * the power goes in between, e2fsck just finds an unused inode.
     */
    if ((ext2_put_inode(mp, *inode_num, inode) != OK) ||
        (ext2_add_dirent(mp, parent_inode, name, *inode_num, EXT2_FT_REG_FILE) != OK)) {
        int err = errno;

        ext2_free_inode(mp, *inode_num, NO);
        errno = err;
        return NOT_OK;
    }

    return OK;
}

/*
 * Open a file or directory on an ext2 filesystem.
 *
 * name is relative to the root of the mount and already normalised by
 * the VFS layer: no leading slash, no "." or ".." components, no
 * repeated slashes. "" means the root directory of the mount.
 *
 * With O_CREAT, a missing file is created, with the permissions the VFS
 * put in file->mode. O_TRUNC empties an existing file.
 */
int ext2_open(vfile_t *file, const char *name, int flags, vfile_t *cwd) {
    ext2_file_t *filep = &file->private.data.ext2_file_inf;
    char path[PATH_MAX];
    char *dirv[EXT2_MAX_DIR_DEPTH];
    int dirc = 0;
    uint32_t inode_num = EXT2_ROOT_INO;
    ext2_inode_t inode;

    bool_t writing = (flags & (O_WRONLY | O_TRUNC | O_APPEND))?YES:NO;
    uint32_t create_mode = file->mode;

    (void)cwd;      // The VFS always gives us a path from the mount's root

    if (file->mp == NULL) {
        kprintf("ext2_open: NULL vmp_t in vfile_t\n");
        errno = EBADF;

        return NOT_OK;
    }

    if ((writing == YES) && (file->mp->private.data.ext2_fs_inf.writable != YES)) {
        errno = EROFS;
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
            // Missing - create it if asked to and it's the last part of the path
            if ((flags & O_CREAT) && (i == dirc - 1)) {
                if (file->mp->private.data.ext2_fs_inf.writable != YES) {
                    errno = EROFS;
                    return NOT_OK;
                }

                if (create_file(file->mp, parent_inode_num, dirv[i], create_mode, &inode_num, &inode) != OK) {
                    return NOT_OK;
                }

                continue;
            }

            // kprintf("ext2_open: couldn't find '%s' in directory with inode %d\n", dirv[i], parent_inode_num);
            errno = ENOENT;
            return NOT_OK;
        }

        // O_CREAT | O_EXCL insists on making a new file
        if ((i == dirc - 1) && ((flags & (O_CREAT | O_EXCL)) == (O_CREAT | O_EXCL))) {
            errno = EEXIST;
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

    if ((writing == YES) && S_ISDIR(inode.i_mode)) {
        errno = EISDIR;
        return NOT_OK;
    }

    if (ext2_init_block_follower(&filep->bf, file->mp, inode_num) == NOT_OK) {
        errno = EIO;
        return NOT_OK;
    }

    filep->mp = file->mp;
    filep->offset = 0;
    filep->inode_dirty = NO;
    filep->last_block = 0;

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

    // O_TRUNC empties an existing regular file
    if ((flags & O_TRUNC) && (flags & O_WRONLY) && S_ISREG(inode.i_mode) && (inode.i_size > 0)) {
        if (ext2_truncate(file) != OK) {
            return NOT_OK;
        }
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

int ext2_close(vfile_t *file) {
    int res = OK;

    // Write the inode if it changed, then the free counts
    if (ext2_flush_inode(file) != OK) {
        res = NOT_OK;
    }

    if (ext2_sync(file->mp) != OK) {
        res = NOT_OK;
    }

    return res;
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

    // Go straight to block_num - the next read carries on after it
    ext2_set_block_follower(&filep->bf, block_num);
    block = ext2_get_next_block_num(&filep->bf);

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

/*
 * Add what ext2 knows about an open file to *st. The VFS has already
 * filled in the common fields; the inode was read when the file was
 * opened (and byte-swapped), so this needs no disk access.
 */
int ext2_stat(vfile_t *file, struct stat *st) {
    ext2_block_follower_t *bf = &file->private.data.ext2_file_inf.bf;
    ext2_inode_t *in = &bf->inode;

    st->st_ino = bf->inode_num;
    st->st_nlink = in->i_links_count;
    st->st_uid = in->i_uid;
    st->st_gid = in->i_gid;
    st->st_atime = in->i_atime;
    st->st_mtime = in->i_mtime;
    st->st_ctime = in->i_ctime;
    st->st_blocks = in->i_blocks;       // ext2 already counts 512-byte sectors

    return OK;
}
