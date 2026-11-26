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


/*
 * Removing from ext2: deleting files (unlink) and empty directories
 * (rmdir), and changing permissions (chmod).
 *
 * Names are relative to the root of the mount and already normalised by
 * the VFS layer, as for ext2_open().
 *
 * The directory entry goes first, then the inode and its blocks are
 * freed: if the power goes in between, e2fsck just finds an inode that
 * nothing points to, and frees it.
 *
 * A file that's open (or the current directory) can't be removed: here
 * an open file keeps its own copy of the inode, which nothing would stop
 * it writing back over the freed one.
 */

#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <extras.h>
#include <sys/stat.h>

#include "micromon.h"

#define SECTORS_PER_BLOCK_  (BLOCK_DEVICE_BLOCK_SIZE / 512)

// Extended attribute block header
#define EXT2_XATTR_MAGIC    0xEA020000

static void put_le16(uint8_t *p, uint16_t val) {
    p[0] = (uint8_t)val;
    p[1] = (uint8_t)(val >> 8);
}

static void put_le32(uint8_t *p, uint32_t val) {
    p[0] = (uint8_t)val;
    p[1] = (uint8_t)(val >> 8);
    p[2] = (uint8_t)(val >> 16);
    p[3] = (uint8_t)(val >> 24);
}

static int check_writable(vmp_t *mp) {
    if (mp->private.data.ext2_fs_inf.writable != YES) {
        errno = EROFS;
        return NOT_OK;
    }

    return OK;
}

/*
 * Find the last component of 'name': the directory it's in (*parent),
 * its own inode (*inode_num) and its name within that directory (leaf,
 * which has room for EXT2_MAX_LEN + 1). name must not be "".
 */
static int lookup(vmp_t *mp, const char *name, uint32_t *parent, uint32_t *inode_num, char *leaf) {
    char path[PATH_MAX];
    char *dirv[EXT2_MAX_DIR_DEPTH];
    int dirc;
    uint32_t dir = EXT2_ROOT_INO;

    if (strlen(name) >= sizeof(path)) {
        errno = ENAMETOOLONG;
        return NOT_OK;
    }

    // split_str() writes into the string it splits, so work on a copy
    strcpy(path, name);
    dirc = split_str(path, '/', dirv, EXT2_MAX_DIR_DEPTH);
    if (dirc <= 0) {
        errno = ENOENT;
        return NOT_OK;
    }

    for (int i=0; i<dirc; i++) {
        ext2_inode_t inode;
        uint32_t found;

        // We can only look inside directories
        if (ext2_get_inode(mp, dir, &inode) != OK) {
            errno = EIO;
            return NOT_OK;
        }

        if (!S_ISDIR(inode.i_mode)) {
            errno = ENOTDIR;
            return NOT_OK;
        }

        found = e2_search(mp, dir, dirv[i], NULL);
        if (found == 0) {
            errno = ENOENT;
            return NOT_OK;
        }

        if (i == dirc - 1) {
            if (strlen(dirv[i]) > EXT2_MAX_LEN) {
                errno = ENAMETOOLONG;
                return NOT_OK;
            }

            *parent = dir;
            *inode_num = found;
            strcpy(leaf, dirv[i]);
            return OK;
        }

        dir = found;
    }

    errno = ENOENT;
    return NOT_OK;
}

/*
 * Is the inode open, or the current directory? Every open file has its
 * own in-memory copy of its inode.
 */
static bool_t in_use(vmp_t *mp, uint32_t inode_num) {
    int pos = 0;
    vfile_t *file;

    while ((file = vfs_next_open_file(&pos)) != NULL) {
        if ((file->mp == mp) && (file->fs == mp->fs_driver) &&
            (file->private.data.ext2_file_inf.bf.inode_num == inode_num)) {
            return YES;
        }
    }

    return NO;
}

/*
 * Visit each directory entry of dir_inode in turn, with the block it's in
 * loaded in mp->block_buffer. visit() returns YES to stop.
 */
typedef bool_t (*dirent_visitor_t)(vmp_t *mp, uint8_t *ent, uint8_t *prev, void *arg);

static int scan_dir(vmp_t *mp, uint32_t dir_inode, dirent_visitor_t visit, void *arg, bool_t *stopped) {
    ext2_block_follower_t bf;

    *stopped = NO;

    if (ext2_init_block_follower(&bf, mp, dir_inode) != OK) {
        errno = EIO;
        return NOT_OK;
    }

    for (uint32_t block = ext2_get_next_block_num(&bf); block != 0; block = ext2_get_next_block_num(&bf)) {
        uint32_t off = 0;
        uint8_t *prev = NULL;

        if (ext2_read_fs_block(mp, block, NO) != OK) {
            errno = EIO;
            return NOT_OK;
        }

        while (off < BLOCK_DEVICE_BLOCK_SIZE) {
            uint8_t *ent = &mp->block_buffer[off];
            uint16_t rec_len = le16(&ent[4]);

            if ((rec_len < 8) || (off + rec_len > BLOCK_DEVICE_BLOCK_SIZE)) {
                kprintf("ext2: corrupt directory entry in inode %d, block %d\n", dir_inode, block);
                errno = EIO;
                return NOT_OK;
            }

            if (visit(mp, ent, prev, arg) == YES) {
                *stopped = YES;
                return OK;
            }

            prev = ent;
            off += rec_len;
        }
    }

    return OK;
}

static bool_t is_name(const uint8_t *ent, const char *name) {
    size_t len = strlen(name);

    return ((le32(&ent[0]) != 0) && (ent[6] == len) && (memcmp(&ent[8], name, len) == 0))?YES:NO;
}

typedef struct {
    const char *name;
    int res;
} remove_arg_t;

// Remove the entry: fold its space into the one before, or mark it unused
static bool_t remove_visitor(vmp_t *mp, uint8_t *ent, uint8_t *prev, void *arg) {
    remove_arg_t *ra = (remove_arg_t *)arg;

    if (is_name(ent, ra->name) == NO) {
        return NO;
    }

    if (prev != NULL) {
        put_le16(&prev[4], le16(&prev[4]) + le16(&ent[4]));
    }
    else {
        put_le32(&ent[0], 0);
    }

    ra->res = ext2_write_fs_block(mp);
    if (ra->res != OK) {
        mp->block_in_buffer_valid = NO;
    }

    return YES;
}

static int remove_dirent(vmp_t *mp, uint32_t dir_inode, const char *name) {
    remove_arg_t arg;
    ext2_inode_t dir;
    bool_t found;

    arg.name = name;
    arg.res = NOT_OK;

    if (scan_dir(mp, dir_inode, remove_visitor, &arg, &found) != OK) {
        return NOT_OK;
    }

    if (found == NO) {
        errno = ENOENT;
        return NOT_OK;
    }

    if (arg.res != OK) {
        errno = EIO;
        return NOT_OK;
    }

    // The directory has changed
    if (ext2_get_inode(mp, dir_inode, &dir) != OK) {
        errno = EIO;
        return NOT_OK;
    }

    dir.i_mtime = dir.i_ctime = ext2_now();

    return ext2_put_inode(mp, dir_inode, &dir);
}

// Stops at anything but "." and ".."
static bool_t not_empty_visitor(vmp_t *mp, uint8_t *ent, uint8_t *prev, void *arg) {
    (void)mp;
    (void)prev;
    (void)arg;

    if (le32(&ent[0]) == 0) {
        return NO;
    }

    if ((ent[6] == 1) && (ent[8] == '.')) {
        return NO;
    }

    if ((ent[6] == 2) && (ent[8] == '.') && (ent[9] == '.')) {
        return NO;
    }

    return YES;
}

/*
 * Drop the inode's reference to its extended attribute block, freeing the
 * block if nothing else shares it.
 */
static int release_xattr(vmp_t *mp, ext2_inode_t *in) {
    static uint8_t block[BLOCK_DEVICE_BLOCK_SIZE];
    uint32_t refs;

    if (in->i_file_acl == 0) {
        return OK;
    }

    if (ext2_read_block(mp, in->i_file_acl, block) != OK) {
        errno = EIO;
        return NOT_OK;
    }

    refs = le32(&block[4]);

    if ((le32(&block[0]) == EXT2_XATTR_MAGIC) && (refs > 1)) {
        put_le32(&block[4], refs - 1);

        if (ext2_write_block(mp, in->i_file_acl, block) != OK) {
            errno = EIO;
            return NOT_OK;
        }
    }
    else if (ext2_free_block(mp, in->i_file_acl) != OK) {
        return NOT_OK;
    }

    in->i_file_acl = 0;
    if (in->i_blocks >= SECTORS_PER_BLOCK_) {
        in->i_blocks -= SECTORS_PER_BLOCK_;
    }

    return OK;
}

/*
 * Free an inode that nothing points to any more, and its blocks. Only
 * regular files, directories and long symlinks have data blocks: a short
 * symlink keeps its target, and a device its numbers, in i_block.
 */
static int release_inode(vmp_t *mp, uint32_t inode_num, ext2_inode_t *in) {
    ext2_sb_t *sb = &mp->private.data.ext2_fs_inf.sb;
    uint32_t xattr_sectors = (in->i_file_acl != 0)?SECTORS_PER_BLOCK_:0;
    bool_t is_dir = S_ISDIR(in->i_mode)?YES:NO;
    bool_t has_blocks = (S_ISREG(in->i_mode) || S_ISDIR(in->i_mode) ||
                         (S_ISLNK(in->i_mode) && (in->i_blocks > xattr_sectors)))?YES:NO;
    int res = OK;

    if ((has_blocks == YES) && (ext2_free_data_blocks(mp, in) != OK)) {
        res = NOT_OK;
    }

    if (release_xattr(mp, in) != OK) {
        res = NOT_OK;
    }

    /*
     * e2fsck takes a deletion time less than the number of inodes as a
     * link in the orphan list, so without the RTC use the last write time
     * (or failing that, the inode count)
     */
    in->i_links_count = 0;
    in->i_dtime = ext2_now();
    if (in->i_dtime < sb->s_inodes_count) {
        in->i_dtime = (sb->s_wtime >= sb->s_inodes_count)?sb->s_wtime:sb->s_inodes_count;
    }

    if (ext2_put_inode(mp, inode_num, in) != OK) {
        res = NOT_OK;
    }

    if (ext2_free_inode(mp, inode_num, is_dir) != OK) {
        res = NOT_OK;
    }

    return res;
}

int ext2_unlink(vmp_t *mp, const char *name) {
    char leaf[EXT2_MAX_LEN + 1];
    uint32_t parent, inode_num;
    ext2_inode_t inode;
    int res;

    if (check_writable(mp) != OK) {
        return NOT_OK;
    }

    if (name[0] == EOS) {
        errno = EISDIR;             // the root directory
        return NOT_OK;
    }

    if (lookup(mp, name, &parent, &inode_num, leaf) != OK) {
        return NOT_OK;
    }

    if (ext2_get_inode(mp, inode_num, &inode) != OK) {
        errno = EIO;
        return NOT_OK;
    }

    if (S_ISDIR(inode.i_mode)) {
        errno = EISDIR;
        return NOT_OK;
    }

    if (in_use(mp, inode_num) == YES) {
        errno = EBUSY;
        return NOT_OK;
    }

    if (remove_dirent(mp, parent, leaf) != OK) {
        return NOT_OK;
    }

    // Another name (a hard link) may still point to it
    if (inode.i_links_count > 1) {
        inode.i_links_count--;
        inode.i_ctime = ext2_now();
        res = ext2_put_inode(mp, inode_num, &inode);
    }
    else {
        res = release_inode(mp, inode_num, &inode);
    }

    if (ext2_sync(mp) != OK) {
        res = NOT_OK;
    }

    return res;
}

int ext2_rmdir(vmp_t *mp, const char *name) {
    char leaf[EXT2_MAX_LEN + 1];
    uint32_t parent, inode_num;
    ext2_inode_t inode, pinode;
    bool_t not_empty;
    int res;

    if (check_writable(mp) != OK) {
        return NOT_OK;
    }

    if (name[0] == EOS) {
        errno = EBUSY;              // the root directory
        return NOT_OK;
    }

    if (lookup(mp, name, &parent, &inode_num, leaf) != OK) {
        return NOT_OK;
    }

    if (ext2_get_inode(mp, inode_num, &inode) != OK) {
        errno = EIO;
        return NOT_OK;
    }

    if (!S_ISDIR(inode.i_mode)) {
        errno = ENOTDIR;
        return NOT_OK;
    }

    if (in_use(mp, inode_num) == YES) {
        errno = EBUSY;
        return NOT_OK;
    }

    if (scan_dir(mp, inode_num, not_empty_visitor, NULL, &not_empty) != OK) {
        return NOT_OK;
    }

    if (not_empty == YES) {
        errno = ENOTEMPTY;
        return NOT_OK;
    }

    if (remove_dirent(mp, parent, leaf) != OK) {
        return NOT_OK;
    }

    // Its ".." was one of the parent's links
    if (ext2_get_inode(mp, parent, &pinode) != OK) {
        errno = EIO;
        return NOT_OK;
    }

    if (pinode.i_links_count > 0) {
        pinode.i_links_count--;
    }

    res = ext2_put_inode(mp, parent, &pinode);

    if (release_inode(mp, inode_num, &inode) != OK) {
        res = NOT_OK;
    }

    if (ext2_sync(mp) != OK) {
        res = NOT_OK;
    }

    return res;
}

int ext2_chmod(vmp_t *mp, const char *name, mode_t mode) {
    char leaf[EXT2_MAX_LEN + 1];
    uint32_t parent, inode_num = EXT2_ROOT_INO;
    ext2_inode_t inode;
    int pos = 0;
    vfile_t *file;

    if (check_writable(mp) != OK) {
        return NOT_OK;
    }

    if ((name[0] != EOS) && (lookup(mp, name, &parent, &inode_num, leaf) != OK)) {
        return NOT_OK;
    }

    if (ext2_get_inode(mp, inode_num, &inode) != OK) {
        errno = EIO;
        return NOT_OK;
    }

    inode.i_mode = (inode.i_mode & S_IFMT) | (mode & 07777);
    inode.i_ctime = ext2_now();

    if (ext2_put_inode(mp, inode_num, &inode) != OK) {
        return NOT_OK;
    }

    // Open copies of the inode would write the old mode back: update them
    while ((file = vfs_next_open_file(&pos)) != NULL) {
        ext2_file_t *fp = &file->private.data.ext2_file_inf;

        if ((file->mp == mp) && (file->fs == mp->fs_driver) && (fp->bf.inode_num == inode_num)) {
            fp->bf.inode.i_mode = inode.i_mode;
            fp->bf.inode.i_ctime = inode.i_ctime;
            file->mode = inode.i_mode;
        }
    }

    return OK;
}
