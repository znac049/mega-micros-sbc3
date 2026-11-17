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
 * ext2 block and inode allocation.
 *
 * Free space is recorded three times: a bit per block (or inode) in each
 * group's bitmap, a free count in each group descriptor, and a total in
 * the superblock. The bitmaps are the real record - they're written as
 * soon as they change. The counts are kept up to date in memory and
 * written by ext2_sync(), so that writing a file doesn't rewrite the
 * superblock for every block. If the power goes before a sync, only the
 * counts are wrong, and e2fsck puts those right without asking.
 */

#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <machine.h>

#include "micromon.h"

// Byte offsets of the fields we update, within the on-disk structures
#define SB_OFFSET               1024    // superblock within block 0 (2KB blocks)
#define SB_FREE_BLOCKS          12
#define SB_FREE_INODES          16
#define BG_FREE_BLOCKS          12
#define BG_FREE_INODES          14
#define BG_USED_DIRS            16

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

static ext2_fs_t *get_fs(vmp_t *mp) {
    return &mp->private.data.ext2_fs_inf;
}

/*
 * Decide at mount time whether this filesystem may be written. Returns
 * NULL if it may, otherwise the reason it's read-only.
 */
const char *ext2_check_writable(vmp_t *mp) {
    ext2_sb_t *sb = &get_fs(mp)->sb;

    if (mp->dev_driver->write_block == NULL) {
        return "the device is read-only";
    }

    if (sb->s_rev_level >= 1) {
        if (sb->s_feature_incompat & ~EXT2_FEATURE_INCOMPAT_SUPP) {
            return "it uses features this code can't write";
        }

        if (sb->s_feature_ro_compat & ~EXT2_FEATURE_RO_COMPAT_SUPP) {
            return "it uses features this code can't write";
        }
    }

    if ((sb->s_state & EXT2_VALID_FS) == 0) {
        return "it wasn't cleanly unmounted - check it with e2fsck";
    }

    if (sb->s_state & EXT2_ERROR_FS) {
        return "it has errors - check it with e2fsck";
    }

    return NULL;
}

static int check_writable(ext2_fs_t *fs) {
    if (fs->writable != YES) {
        errno = EROFS;
        return NOT_OK;
    }

    return OK;
}

// Number of blocks in a group (the last one may be short)
static uint32_t blocks_in_group(ext2_fs_t *fs, uint32_t group) {
    uint32_t first = fs->sb.s_first_data_block + group * fs->sb.s_blocks_per_group;
    uint32_t left = fs->sb.s_blocks_count - first;

    return (left < fs->sb.s_blocks_per_group)?left:fs->sb.s_blocks_per_group;
}

// Find a clear bit in bitmap[from..to), or return -1
static int find_clear_bit(const uint8_t *bitmap, uint32_t from, uint32_t to) {
    for (uint32_t i=from; i<to; i++) {
        if ((i % 8 == 0) && (bitmap[i/8] == 0xff) && (i + 8 <= to)) {
            i += 7;                     // whole byte in use - skip it
            continue;
        }

        if ((bitmap[i/8] & (1 << (i % 8))) == 0) {
            return (int)i;
        }
    }

    return -1;
}

/*
 * Set (or clear) bit 'bit' of the bitmap in 'bitmap_block', and write it
 * back. Returns NOT_OK if the bit was already in that state - meaning the
 * bitmap and the free counts disagree.
 */
static int change_bit(vmp_t *mp, uint32_t bitmap_block, uint32_t bit, bool_t set) {
    uint8_t mask = 1 << (bit % 8);
    uint8_t *byte;

    if (ext2_read_fs_block(mp, bitmap_block, NO) != OK) {
        errno = EIO;
        return NOT_OK;
    }

    byte = &mp->block_buffer[bit / 8];

    if (((*byte & mask) != 0) == (set == YES)) {
        kprintf("ext2: bitmap block %d, bit %d is already %s\n", bitmap_block, bit, set?"set":"clear");
        errno = EIO;
        return NOT_OK;
    }

    if (set) {
        *byte |= mask;
    }
    else {
        *byte &= ~mask;
    }

    if (ext2_write_fs_block(mp) != OK) {
        // The buffer no longer matches the disk
        mp->block_in_buffer_valid = NO;
        return NOT_OK;
    }

    return OK;
}

/*
 * Allocate a block, as close to 'goal' as possible (0 for no preference).
 * If 'zero' is YES, the new block is filled with zeros on the disk - needed
 * for indirect blocks and directories. Returns the block number in *block.
 */
int ext2_alloc_block(vmp_t *mp, uint32_t goal, bool_t zero, uint32_t *block) {
    ext2_fs_t *fs = get_fs(mp);
    uint32_t start_group = 0;
    uint32_t start_bit = 0;

    if (check_writable(fs) != OK) {
        return NOT_OK;
    }

    if (fs->sb.s_free_blocks_count == 0) {
        errno = ENOSPC;
        return NOT_OK;
    }

    if ((goal >= fs->sb.s_first_data_block) && (goal < fs->sb.s_blocks_count)) {
        start_group = (goal - fs->sb.s_first_data_block) / fs->sb.s_blocks_per_group;
        start_bit = (goal - fs->sb.s_first_data_block) % fs->sb.s_blocks_per_group;
    }

    for (uint32_t k=0; k<fs->num_blockgroups; k++) {
        uint32_t group = (start_group + k) % fs->num_blockgroups;
        ext2_bg_t *bg = &fs->bgdt[group];
        uint32_t nbits = blocks_in_group(fs, group);
        uint32_t from = (k == 0)?start_bit:0;
        int bit;

        if (bg->bg_free_blocks_count == 0) {
            continue;
        }

        if (ext2_read_fs_block(mp, bg->bg_block_bitmap, NO) != OK) {
            errno = EIO;
            return NOT_OK;
        }

        bit = find_clear_bit(mp->block_buffer, from, nbits);
        if ((bit < 0) && (from > 0)) {
            bit = find_clear_bit(mp->block_buffer, 0, from);
        }

        if (bit < 0) {
            kprintf("ext2: group %d says it has free blocks, but its bitmap is full\n", group);
            continue;
        }

        if (change_bit(mp, bg->bg_block_bitmap, bit, YES) != OK) {
            return NOT_OK;
        }

        bg->bg_free_blocks_count--;
        fs->sb.s_free_blocks_count--;
        fs->counts_dirty = YES;

        *block = fs->sb.s_first_data_block + group * fs->sb.s_blocks_per_group + bit;

        if (zero == YES) {
            // Use the mount's buffer as the source of zeros
            memset(mp->block_buffer, 0, BLOCK_DEVICE_BLOCK_SIZE);
            mp->block_num_in_buffer = *block;
            mp->block_in_buffer_valid = YES;

            if (ext2_write_fs_block(mp) != OK) {
                mp->block_in_buffer_valid = NO;
                return NOT_OK;
            }
        }

        return OK;
    }

    errno = ENOSPC;
    return NOT_OK;
}

int ext2_free_block(vmp_t *mp, uint32_t block) {
    ext2_fs_t *fs = get_fs(mp);
    uint32_t group, bit;

    if (check_writable(fs) != OK) {
        return NOT_OK;
    }

    if ((block < fs->sb.s_first_data_block) || (block >= fs->sb.s_blocks_count)) {
        kprintf("ext2_free_block: block %d is out of range\n", block);
        errno = EINVAL;
        return NOT_OK;
    }

    group = (block - fs->sb.s_first_data_block) / fs->sb.s_blocks_per_group;
    bit = (block - fs->sb.s_first_data_block) % fs->sb.s_blocks_per_group;

    if (change_bit(mp, fs->bgdt[group].bg_block_bitmap, bit, NO) != OK) {
        return NOT_OK;
    }

    fs->bgdt[group].bg_free_blocks_count++;
    fs->sb.s_free_blocks_count++;
    fs->counts_dirty = YES;

    return OK;
}

/*
 * Allocate an inode. Files go near their directory (same group if
 * possible); new directories go in the group with the most free inodes,
 * to spread them out. The inode itself isn't written - the caller fills
 * it in and uses ext2_put_inode().
 */
int ext2_alloc_inode(vmp_t *mp, uint32_t parent_inode, bool_t is_dir, uint32_t *inode_num) {
    ext2_fs_t *fs = get_fs(mp);
    uint32_t ipg = fs->sb.s_inodes_per_group;
    uint32_t first_ino = (fs->sb.s_rev_level >= 1)?fs->sb.s_first_ino:EXT2_GOOD_OLD_FIRST_INO;
    uint32_t start_group = 0;

    if (check_writable(fs) != OK) {
        return NOT_OK;
    }

    if (fs->sb.s_free_inodes_count == 0) {
        errno = ENOSPC;
        return NOT_OK;
    }

    if (is_dir == YES) {
        uint32_t best = 0;

        for (uint32_t g=0; g<fs->num_blockgroups; g++) {
            if (fs->bgdt[g].bg_free_inodes_count > best) {
                best = fs->bgdt[g].bg_free_inodes_count;
                start_group = g;
            }
        }
    }
    else if ((parent_inode > 0) && (parent_inode <= fs->sb.s_inodes_count)) {
        start_group = (parent_inode - 1) / ipg;
    }

    for (uint32_t k=0; k<fs->num_blockgroups; k++) {
        uint32_t group = (start_group + k) % fs->num_blockgroups;
        ext2_bg_t *bg = &fs->bgdt[group];
        uint32_t from = 0;
        int bit;

        if (bg->bg_free_inodes_count == 0) {
            continue;
        }

        // Never hand out the reserved inodes at the start of group 0
        if ((group == 0) && (first_ino > 1)) {
            from = first_ino - 1;
        }

        if (ext2_read_fs_block(mp, bg->bg_inode_bitmap, NO) != OK) {
            errno = EIO;
            return NOT_OK;
        }

        bit = find_clear_bit(mp->block_buffer, from, ipg);
        if (bit < 0) {
            kprintf("ext2: group %d says it has free inodes, but its bitmap is full\n", group);
            continue;
        }

        if (change_bit(mp, bg->bg_inode_bitmap, bit, YES) != OK) {
            return NOT_OK;
        }

        bg->bg_free_inodes_count--;
        fs->sb.s_free_inodes_count--;

        if (is_dir == YES) {
            bg->bg_used_dirs_count++;
        }

        fs->counts_dirty = YES;

        *inode_num = group * ipg + bit + 1;

        return OK;
    }

    errno = ENOSPC;
    return NOT_OK;
}

int ext2_free_inode(vmp_t *mp, uint32_t inode_num, bool_t is_dir) {
    ext2_fs_t *fs = get_fs(mp);
    uint32_t ipg = fs->sb.s_inodes_per_group;
    uint32_t group, bit;

    if (check_writable(fs) != OK) {
        return NOT_OK;
    }

    if ((inode_num == 0) || (inode_num > fs->sb.s_inodes_count)) {
        kprintf("ext2_free_inode: inode %d is out of range\n", inode_num);
        errno = EINVAL;
        return NOT_OK;
    }

    group = (inode_num - 1) / ipg;
    bit = (inode_num - 1) % ipg;

    if (change_bit(mp, fs->bgdt[group].bg_inode_bitmap, bit, NO) != OK) {
        return NOT_OK;
    }

    fs->bgdt[group].bg_free_inodes_count++;
    fs->sb.s_free_inodes_count++;

    if ((is_dir == YES) && (fs->bgdt[group].bg_used_dirs_count > 0)) {
        fs->bgdt[group].bg_used_dirs_count--;
    }

    fs->counts_dirty = YES;

    return OK;
}

/*
 * Write an inode back to the inode table. Only the fields this code knows
 * about are replaced - anything else in the slot (such as the extra fields
 * of a 256-byte inode) is left as it was.
 */
int ext2_put_inode(vmp_t *mp, uint32_t inode_num, ext2_inode_t *inode) {
    ext2_fs_t *fs = get_fs(mp);
    uint32_t group, index, block_num, offset;

    if (check_writable(fs) != OK) {
        return NOT_OK;
    }

    if ((inode_num == 0) || (inode_num > fs->sb.s_inodes_count)) {
        kprintf("ext2_put_inode: inode %d is out of range\n", inode_num);
        errno = EINVAL;
        return NOT_OK;
    }

    group = (inode_num - 1) / fs->sb.s_inodes_per_group;
    index = (inode_num - 1) % fs->sb.s_inodes_per_group;

    block_num = fs->bgdt[group].bg_inode_table + (index * fs->sb.s_inode_size) / BLOCK_DEVICE_BLOCK_SIZE;
    offset = (index * fs->sb.s_inode_size) % BLOCK_DEVICE_BLOCK_SIZE;

    if (ext2_read_fs_block(mp, block_num, NO) != OK) {
        errno = EIO;
        return NOT_OK;
    }

    // Converting to disk byte order is the same swap as converting from it
    ext2_sanitize_inode(inode, (ext2_inode_t *)&mp->block_buffer[offset]);

    if (ext2_write_fs_block(mp) != OK) {
        mp->block_in_buffer_valid = NO;
        return NOT_OK;
    }

    return OK;
}

/*
 * Write the free counts (group descriptors and superblock) if they've
 * changed since the last sync.
 */
int ext2_sync(vmp_t *mp) {
    ext2_fs_t *fs = get_fs(mp);
    uint32_t per_block = BLOCK_DEVICE_BLOCK_SIZE / sizeof(ext2_bg_t);
    uint32_t table = fs->sb.s_first_data_block + 1;

    if ((fs->writable != YES) || (fs->counts_dirty != YES)) {
        return OK;
    }

    // Group descriptors: patch the counts into each block of the table
    for (uint32_t g=0; g<fs->num_blockgroups; g += per_block) {
        if (ext2_read_fs_block(mp, table + g / per_block, YES) != OK) {
            errno = EIO;
            return NOT_OK;
        }

        for (uint32_t i=g; (i < g + per_block) && (i < fs->num_blockgroups); i++) {
            uint8_t *desc = &mp->block_buffer[(i - g) * sizeof(ext2_bg_t)];

            put_le16(&desc[BG_FREE_BLOCKS], fs->bgdt[i].bg_free_blocks_count);
            put_le16(&desc[BG_FREE_INODES], fs->bgdt[i].bg_free_inodes_count);
            put_le16(&desc[BG_USED_DIRS], fs->bgdt[i].bg_used_dirs_count);
        }

        if (ext2_write_fs_block(mp) != OK) {
            mp->block_in_buffer_valid = NO;
            return NOT_OK;
        }
    }

    // Superblock (in block 0, at byte 1024)
    if (ext2_read_fs_block(mp, 0, YES) != OK) {
        errno = EIO;
        return NOT_OK;
    }

    put_le32(&mp->block_buffer[SB_OFFSET + SB_FREE_BLOCKS], fs->sb.s_free_blocks_count);
    put_le32(&mp->block_buffer[SB_OFFSET + SB_FREE_INODES], fs->sb.s_free_inodes_count);

    if (ext2_write_fs_block(mp) != OK) {
        mp->block_in_buffer_valid = NO;
        return NOT_OK;
    }

    fs->counts_dirty = NO;

    return OK;
}
