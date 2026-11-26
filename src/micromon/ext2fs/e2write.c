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
 * Writing to ext2: mapping file blocks to disk blocks (allocating them as
 * needed), writing file data, truncating, and adding directory entries.
 *
 * The in-memory copy of a file's inode is the one in its block follower
 * (file->private.data.ext2_file_inf.bf.inode). Changes to it are marked
 * with inode_dirty and written by ext2_flush_inode().
 */

#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <machine.h>

#include "micromon.h"

#define PTRS_PER_BLOCK      (BLOCK_DEVICE_BLOCK_SIZE / sizeof(uint32_t))
#define SECTORS_PER_BLOCK_  (BLOCK_DEVICE_BLOCK_SIZE / 512)

// Space a directory entry with a name of this length needs (multiple of 4)
#define DIRENT_LEN(name_len)    ((8 + (name_len) + 3) & ~3)

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

static ext2_file_t *file_inf(vfile_t *file) {
    return &file->private.data.ext2_file_inf;
}

/*
 * The time now, in seconds since 1 January 1970, from the RTC (0 if there
 * isn't one). Days from the date use Howard Hinnant's days_from_civil().
 */
uint32_t ext2_now(void) {
    ds1307_time_t t;
    int32_t y, era;
    uint32_t yoe, doy, doe, m;

    if ((rtc_present != YES) || (ds1307_read_time(&t) < 0)) {
        return 0;
    }

    if ((t.month < 1) || (t.month > 12) || (t.date < 1) || (t.date > 31) ||
        (t.hours > 23) || (t.minutes > 59) || (t.seconds > 59)) {
        return 0;
    }

    y = 2000 + t.year;
    m = t.month;
    y -= (m <= 2)?1:0;
    era = y / 400;
    yoe = (uint32_t)(y - era * 400);
    doy = (153 * (m + ((m > 2)?-3:9)) + 2) / 5 + t.date - 1;
    doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;

    return (uint32_t)(era * 146097 + (int32_t)doe - 719468) * 86400 +
           t.hours * 3600 + t.minutes * 60 + t.seconds;
}

// Write the inode back if it has changed
int ext2_flush_inode(vfile_t *file) {
    ext2_file_t *fp = file_inf(file);

    if (fp->inode_dirty != YES) {
        return OK;
    }

    if (ext2_put_inode(file->mp, fp->bf.inode_num, &fp->bf.inode) != OK) {
        return NOT_OK;
    }

    fp->inode_dirty = NO;

    return OK;
}

static int get_ptr(vmp_t *mp, uint32_t table, uint32_t index, uint32_t *val) {
    if (ext2_read_fs_block(mp, table, NO) != OK) {
        errno = EIO;
        return NOT_OK;
    }

    *val = le32(&mp->block_buffer[index * sizeof(uint32_t)]);

    return OK;
}

static int set_ptr(vmp_t *mp, uint32_t table, uint32_t index, uint32_t val) {
    if (ext2_read_fs_block(mp, table, NO) != OK) {
        errno = EIO;
        return NOT_OK;
    }

    put_le32(&mp->block_buffer[index * sizeof(uint32_t)], val);

    if (ext2_write_fs_block(mp) != OK) {
        mp->block_in_buffer_valid = NO;
        return NOT_OK;
    }

    return OK;
}

// Allocate a block for this file, near its last one (or its inode's group)
static int alloc_for(vfile_t *file, bool_t zero, uint32_t *block) {
    ext2_file_t *fp = file_inf(file);
    ext2_fs_t *fs = &file->mp->private.data.ext2_fs_inf;
    uint32_t goal = fp->last_block;

    if (goal != 0) {
        goal++;
    }
    else {
        uint32_t group = (fp->bf.inode_num - 1) / fs->sb.s_inodes_per_group;

        goal = fs->sb.s_first_data_block + group * fs->sb.s_blocks_per_group;
    }

    if (ext2_alloc_block(file->mp, goal, zero, block) != OK) {
        return NOT_OK;
    }

    fp->last_block = *block;
    fp->bf.inode.i_blocks += SECTORS_PER_BLOCK_;
    fp->inode_dirty = YES;

    return OK;
}

/*
 * One step down the block tree. The pointer is either in the inode
 * (slot != NULL) or entry 'index' of indirect block 'table'. If it's 0 and
 * 'create' is set, allocate a block for it - zeroed if it will be another
 * table of pointers.
 */
static int step(vfile_t *file, uint32_t *slot, uint32_t table, uint32_t index,
                bool_t create, bool_t is_table, uint32_t *child, bool_t *created) {
    ext2_file_t *fp = file_inf(file);

    if (slot != NULL) {
        *child = *slot;
    }
    else if (get_ptr(file->mp, table, index, child) != OK) {
        return NOT_OK;
    }

    if ((*child == 0) && (create == YES)) {
        if (alloc_for(file, is_table, child) != OK) {
            return NOT_OK;
        }

        if (slot != NULL) {
            *slot = *child;
            fp->inode_dirty = YES;
        }
        else if (set_ptr(file->mp, table, index, *child) != OK) {
            return NOT_OK;
        }

        // The follower may have cached a pointer table that just changed
        fp->bf.leaf_id = 0;

        if ((created != NULL) && (is_table == NO)) {
            *created = YES;
        }
    }

    return OK;
}

/*
 * Find the disk block holding logical block n of the file. With 'create',
 * allocate it (and any indirect blocks on the way) if it isn't there yet,
 * setting *created. Otherwise *block is 0 for a hole.
 */
int ext2_bmap(vfile_t *file, uint32_t n, bool_t create, uint32_t *block, bool_t *created) {
    const uint32_t P = PTRS_PER_BLOCK;
    ext2_inode_t *in = &file_inf(file)->bf.inode;
    uint32_t table = 0;
    uint32_t slot;
    uint32_t idx[3];            // index at each level of indirection
    int levels;

    *block = 0;

    if (created != NULL) {
        *created = NO;
    }

    // Which inode slot, and the index at each level below it
    if (n < EXT2_SNGL_IND) {
        return step(file, &in->i_block[n], 0, 0, create, NO, block, created);
    }

    n -= EXT2_SNGL_IND;

    if (n < P) {
        slot = EXT2_SNGL_IND;
        levels = 1;
        idx[0] = n;
    }
    else if ((n -= P) < P * P) {
        slot = EXT2_DBL_IND;
        levels = 2;
        idx[0] = n / P;
        idx[1] = n % P;
    }
    else if ((n -= P * P) < P * P * P) {
        slot = EXT2_TRIP_IND;
        levels = 3;
        idx[0] = n / (P * P);
        idx[1] = (n / P) % P;
        idx[2] = n % P;
    }
    else {
        errno = EFBIG;
        return NOT_OK;
    }

    // The top table hangs off the inode
    if (step(file, &in->i_block[slot], 0, 0, create, YES, &table, NULL) != OK) {
        return NOT_OK;
    }

    // Walk down through the tables; the last step finds the data block
    for (int l=0; l<levels; l++) {
        bool_t last = (l == levels - 1)?YES:NO;
        uint32_t child = 0;

        if (table == 0) {
            return OK;                  // a hole (only possible without create)
        }

        if (step(file, NULL, table, idx[l], create, last?NO:YES, &child, last?created:NULL) != OK) {
            return NOT_OK;
        }

        table = child;
    }

    *block = table;

    return OK;
}

/*
 * Write count bytes at the file's position. Each block is written to the
 * card as soon as it's complete or the call ends; the inode (size, times,
 * block list) is written at the end of the call.
 *
 * Uses the file's buffer the same way reading does - position is where we
 * are in the file, index where that is in buffer[] - so reads and writes
 * can be mixed freely.
 */
int ext2_write(vfile_t *file, const char *buff, size_t count) {
    ext2_file_t *fp = file_inf(file);
    ext2_fs_t *fs = &file->mp->private.data.ext2_fs_inf;
    ext2_inode_t *in = &fp->bf.inode;
    size_t done = 0;

    if (fs->writable != YES) {
        errno = EROFS;
        return NOT_OK;
    }

    while (done < count) {
        uint32_t pos = (uint32_t)file->position;
        uint32_t n = pos / BLOCK_DEVICE_BLOCK_SIZE;
        uint32_t off = pos % BLOCK_DEVICE_BLOCK_SIZE;
        uint32_t chunk = BLOCK_DEVICE_BLOCK_SIZE - off;
        bool_t have, created;
        uint32_t block;

        if (chunk > count - done) {
            chunk = count - done;
        }

        // Does the buffer already hold this block?
        have = (file->count > 0) && ((uint32_t)(file->position - file->index) == n * BLOCK_DEVICE_BLOCK_SIZE);

        if (ext2_bmap(file, n, YES, &block, &created) != OK) {
            break;
        }

        if (have == NO) {
            if ((created == YES) || (chunk == BLOCK_DEVICE_BLOCK_SIZE)) {
                // New block, or we're replacing all of it
                memset(file->buffer, 0, BLOCK_DEVICE_BLOCK_SIZE);
            }
            else if (ext2_read_block(file->mp, block, (uint8_t *)file->buffer) != OK) {
                errno = EIO;
                break;
            }
        }

        memcpy(&file->buffer[off], &buff[done], chunk);

        if (ext2_write_block(file->mp, block, (uint8_t *)file->buffer) != OK) {
            break;
        }

        file->index = off + chunk;
        file->count = BLOCK_DEVICE_BLOCK_SIZE;
        file->position = pos + chunk;

        // A following read carries on from the next block
        ext2_set_block_follower(&fp->bf, n + 1);

        if ((uint32_t)file->position > file->size) {
            file->size = file->position;
            in->i_size = file->position;
            fp->inode_dirty = YES;
        }

        done += chunk;
    }

    if (done > 0) {
        in->i_mtime = in->i_ctime = ext2_now();
        fp->inode_dirty = YES;
        file->ateof = NO;
    }

    if (ext2_flush_inode(file) != OK) {
        return NOT_OK;
    }

    return (done > 0)?(int)done:NOT_OK;
}

/*
 * Free a block and, if it's a table of pointers ('depth' levels above the
 * data), everything it points to. Each depth has its own scratch buffer,
 * because freeing uses the mount's block buffer.
 */
static uint8_t scratch[3][BLOCK_DEVICE_BLOCK_SIZE];

static int free_tree(vmp_t *mp, uint32_t block, int depth) {
    if (block == 0) {
        return OK;
    }

    if (depth > 0) {
        uint8_t *table = scratch[depth - 1];

        if (ext2_read_block(mp, block, table) != OK) {
            errno = EIO;
            return NOT_OK;
        }

        for (uint32_t i=0; i<PTRS_PER_BLOCK; i++) {
            if (free_tree(mp, le32(&table[i * sizeof(uint32_t)]), depth - 1) != OK) {
                return NOT_OK;
            }
        }
    }

    return ext2_free_block(mp, block);
}

/*
 * Free all of an inode's data blocks, direct and indirect, and clear its
 * block pointers. An extended attribute block, if it has one, is left
 * alone and still counted in i_blocks. The caller writes the inode.
 */
int ext2_free_data_blocks(vmp_t *mp, ext2_inode_t *in) {
    int res = OK;

    for (int i=0; i<EXT2_SNGL_IND; i++) {
        if (free_tree(mp, in->i_block[i], 0) != OK) {
            res = NOT_OK;
        }
    }

    if ((free_tree(mp, in->i_block[EXT2_SNGL_IND], 1) != OK) ||
        (free_tree(mp, in->i_block[EXT2_DBL_IND], 2) != OK) ||
        (free_tree(mp, in->i_block[EXT2_TRIP_IND], 3) != OK)) {
        res = NOT_OK;
    }

    memset(in->i_block, 0, sizeof(in->i_block));
    in->i_blocks = (in->i_file_acl != 0)?SECTORS_PER_BLOCK_:0;

    return res;
}

// Cut a file down to nothing, freeing all its blocks
int ext2_truncate(vfile_t *file) {
    ext2_file_t *fp = file_inf(file);
    ext2_inode_t *in = &fp->bf.inode;
    int res = ext2_free_data_blocks(file->mp, in);

    in->i_size = 0;
    in->i_mtime = in->i_ctime = ext2_now();
    fp->inode_dirty = YES;

    file->size = 0;
    file->position = 0;
    file->index = file->count = 0;
    fp->last_block = 0;
    ext2_reset_block_follower(&fp->bf);

    if (ext2_flush_inode(file) != OK) {
        res = NOT_OK;
    }

    return res;
}

static void fill_dirent(uint8_t *ent, uint32_t inode_num, const char *name, uint8_t file_type, bool_t has_ftype) {
    size_t len = strlen(name);

    put_le32(&ent[0], inode_num);
    ent[6] = (uint8_t)len;
    ent[7] = (has_ftype == YES)?file_type:0;    // otherwise the high byte of a 16-bit name length
    memcpy(&ent[8], name, len);
}

/*
 * Add an entry for 'name' -> inode_num to directory dir_inode. Uses the
 * first gap big enough (splitting an entry if need be), or adds a block
 * to the directory. The caller has checked the name isn't already there.
 */
int ext2_add_dirent(vmp_t *mp, uint32_t dir_inode, const char *name, uint32_t inode_num, uint8_t file_type) {
    static vfile_t dir;             // ~2.7KB, so not on the stack
    ext2_fs_t *fs = &mp->private.data.ext2_fs_inf;
    ext2_inode_t *in = &file_inf(&dir)->bf.inode;
    bool_t has_ftype = ((fs->sb.s_rev_level >= 1) &&
                        (fs->sb.s_feature_incompat & EXT2_FEATURE_INCOMPAT_FILETYPE))?YES:NO;
    uint16_t need = DIRENT_LEN(strlen(name));
    uint32_t num_blocks;
    uint32_t block;
    bool_t created;

    memset(&dir, 0, sizeof(dir));
    dir.mp = mp;

    if (ext2_init_block_follower(&file_inf(&dir)->bf, mp, dir_inode) != OK) {
        errno = EIO;
        return NOT_OK;
    }

    num_blocks = in->i_size / BLOCK_DEVICE_BLOCK_SIZE;

    for (uint32_t n=0; n<num_blocks; n++) {
        uint32_t off = 0;

        if ((ext2_bmap(&dir, n, NO, &block, NULL) != OK) || (block == 0)) {
            continue;
        }

        if (ext2_read_fs_block(mp, block, NO) != OK) {
            errno = EIO;
            return NOT_OK;
        }

        while (off < BLOCK_DEVICE_BLOCK_SIZE) {
            uint8_t *ent = &mp->block_buffer[off];
            uint32_t ent_inode = le32(&ent[0]);
            uint16_t rec_len = le16(&ent[4]);
            uint16_t used = (ent_inode != 0)?DIRENT_LEN(ent[6]):0;

            if ((rec_len < 8) || (off + rec_len > BLOCK_DEVICE_BLOCK_SIZE)) {
                kprintf("ext2: corrupt directory entry in inode %d, block %d\n", dir_inode, block);
                errno = EIO;
                return NOT_OK;
            }

            if (rec_len - used >= need) {
                if (ent_inode != 0) {
                    // Split: shrink this entry to what it uses, new one after it
                    put_le16(&ent[4], used);
                    ent = &ent[used];
                    put_le16(&ent[4], rec_len - used);
                }

                fill_dirent(ent, inode_num, name, file_type, has_ftype);

                if (ext2_write_fs_block(mp) != OK) {
                    mp->block_in_buffer_valid = NO;
                    return NOT_OK;
                }

                goto added;
            }

            off += rec_len;
        }
    }

    // No room: give the directory another block holding just this entry
    if (ext2_bmap(&dir, num_blocks, YES, &block, &created) != OK) {
        return NOT_OK;
    }

    memset(mp->block_buffer, 0, BLOCK_DEVICE_BLOCK_SIZE);
    mp->block_num_in_buffer = block;
    mp->block_in_buffer_valid = YES;
    put_le16(&mp->block_buffer[4], BLOCK_DEVICE_BLOCK_SIZE);
    fill_dirent(mp->block_buffer, inode_num, name, file_type, has_ftype);

    if (ext2_write_fs_block(mp) != OK) {
        mp->block_in_buffer_valid = NO;
        return NOT_OK;
    }

    in->i_size += BLOCK_DEVICE_BLOCK_SIZE;
    file_inf(&dir)->inode_dirty = YES;

added:
    /*
     * A hashed (indexed) directory's index doesn't know about the new
     * entry, so stop using it - the directory still works as a plain list,
     * and e2fsck accepts that.
     */
    if (in->i_flags & EXT2_INDEX_FL) {
        in->i_flags &= ~EXT2_INDEX_FL;
    }

    in->i_mtime = in->i_ctime = ext2_now();
    file_inf(&dir)->inode_dirty = YES;

    return ext2_flush_inode(&dir);
}
