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
#include <machine.h>
#include <errno.h>
#include <string.h>
#include <extras.h>

#include "micromon.h"

#define PTRS_PER_BLOCK  (BLOCK_DEVICE_BLOCK_SIZE / sizeof(uint32_t))

int ext2_read_block(vmp_t *mp, uint32_t block_num, uint8_t *buffer) {
    // printk("ext2_read_block %d, device='%s%d'\n", block_num, mp->dev_driver->name, mp->subdev);

    return bd_read(mp->dev_driver, block_num, buffer);
}

/*
 * Write a block from any buffer. If that block is also in the mount's
 * block buffer, refresh the copy there so later cached reads see the
 * new contents.
 */
int ext2_write_block(vmp_t *mp, uint32_t block_num, uint8_t *buffer) {
    if (bd_write(mp->dev_driver, block_num, buffer) == NOT_OK) {
        return NOT_OK;
    }

    if ((mp->block_in_buffer_valid == YES) && (mp->block_num_in_buffer == block_num) &&
        (buffer != mp->block_buffer)) {
        memcpy(mp->block_buffer, buffer, BLOCK_DEVICE_BLOCK_SIZE);
    }

    return OK;
}

/*
 * Write the mount's block buffer back to the block it holds (after
 * changing it in place - a bitmap, an inode table block, ...).
 */
int ext2_write_fs_block(vmp_t *mp) {
    if (mp->block_in_buffer_valid != YES) {
        errno = EINVAL;
        return NOT_OK;
    }

    return ext2_write_block(mp, mp->block_num_in_buffer, mp->block_buffer);
}

// Read a block into the block buffer associated with the mount-point
int ext2_read_fs_block(vmp_t *mp, uint32_t block_num, uint8_t force_read) {
    int res;
    
    // Do we already have that block?
    if ((force_read == NO) && 
        (mp->block_num_in_buffer == block_num) &&
        (mp->block_in_buffer_valid == YES)) {
        // printf("block %d already in the buffer\n", block_num);
        return OK;
    }

    res = ext2_read_block(mp, block_num, mp->block_buffer);
    
    if (res == OK) {
        mp->block_num_in_buffer = block_num;
        mp->block_in_buffer_valid = YES;
    }
    else {
        mp->block_in_buffer_valid = NO;
    }

    return res;
}

int ext2_read_blocks(vmp_t *mp, uint32_t block_num, int num_blocks, uint8_t *buffer) {
    uint8_t *buf = buffer;

    for (int i=0; i<num_blocks; i++) {
        if (ext2_read_block(mp, block_num+i, buf) != 0) {
            return i;
        }

        buf += BLOCK_DEVICE_BLOCK_SIZE;
    }

    return num_blocks;
}

void ext2_dump_block_follower(ext2_block_follower_t *bf) {
    printk("\nBlock Follower (0x%08x):\n", bf);

    if (bf == NULL) {
        return;
    }

    printk("  inode_num: %d\n", bf->inode_num);
    printk("  inode: 0x%08x\n", &bf->inode);
    printk("  next_block: %d\n", bf->next_block);
    printk("  leaf_id: %d\n", bf->leaf_id);
    printk("  leaf_block: %d\n", bf->leaf_block);
} 

void ext2_reset_block_follower(ext2_block_follower_t *bf) {
    bf->next_block = 0;
    bf->leaf_id = 0;
    bf->leaf_block = 0;
}

int ext2_init_block_follower(ext2_block_follower_t *bf, vmp_t *mp, uint32_t inode_num) {
    bf->inode_num = inode_num;
    bf->mp = mp;

    ext2_reset_block_follower(bf);

    // printk("ext2_init_block_follower: get inode %d\n", inode_num);
    if (ext2_get_inode(bf->mp, inode_num, &bf->inode) != OK) {
        return NOT_OK;
    }

    // dump_ext2_inode(&bf->inode, inode_num);

    return OK;
}

/*
 * Read entry 'index' from the indirect block 'table_block'. Block 0
 * means "not allocated" (a hole), and so does everything under it.
 *
 * The table is read into the mount's block buffer, which remembers
 * what it holds, so repeated lookups in the same table don't re-read it.
 */
static uint32_t read_block_ptr(vmp_t *mp, uint32_t table_block, uint32_t index) {
    if (table_block == 0) {
        return 0;
    }

    if (ext2_read_fs_block(mp, table_block, NO) != OK) {
        kprintf("ext2: failed to read indirect block %d\n", table_block);
        return 0;
    }

    // Block pointers are little-endian on disk
    return le32(&mp->block_buffer[index * sizeof(uint32_t)]);
}

/*
 * Map a logical block number within the file to a block number on
 * the disk. Returns 0 for a hole or past the end of the addressable range.
 *
 * Blocks 0-11 are listed in the inode. After that, every pointer comes
 * from a "leaf" indirect block holding PTRS_PER_BLOCK pointers. Leaf 0
 * is the single indirect block; leaves 1..P come from the double
 * indirect block; the rest from the triple indirect block.
 */

static uint32_t map_block(ext2_block_follower_t *bf, uint32_t n) {
    const uint32_t P = PTRS_PER_BLOCK;
    ext2_inode_t *in = &bf->inode;
    vmp_t *mp = bf->mp;
    uint32_t leaf;
    uint32_t index;

    if (n < EXT2_SNGL_IND) {
        return in->i_block[n];
    }

    n -= EXT2_SNGL_IND;
    leaf = n / P;           // which leaf indirect block
    index = n % P;          // which pointer within it

    // Work out the leaf's block number, unless we already know it
    if (bf->leaf_id != leaf + 1) {
        uint32_t leaf_block;

        if (leaf == 0) {
            // Single indirect
            leaf_block = in->i_block[EXT2_SNGL_IND];
        }
        else if (leaf <= P) {
            // Double indirect: one level of table above the leaf
            leaf_block = read_block_ptr(mp, in->i_block[EXT2_DBL_IND], leaf - 1);
        }
        else if (leaf <= P + (P * P)) {
            // Triple indirect: two levels of table above the leaf
            uint32_t t = leaf - 1 - P;
            uint32_t mid = read_block_ptr(mp, in->i_block[EXT2_TRIP_IND], t / P);

            leaf_block = read_block_ptr(mp, mid, t % P);
        }
        else {
            return 0;       // beyond what ext2 can address
        }

        bf->leaf_id = leaf + 1;
        bf->leaf_block = leaf_block;
    }

    return read_block_ptr(mp, bf->leaf_block, index);
}

/*
 * Returns the disk block number of the next block of the file (0 for a
 * hole or past the end of the addressable range), and moves on.
 */
uint32_t ext2_get_next_block_num(ext2_block_follower_t *bf) {
    // printk("ext2_get_next_block_num: inode=%d, next=%d\n", bf->inode_num, bf->next_block);

    return map_block(bf, bf->next_block++);
}

/*
 * Position the follower so the next ext2_get_next_block_num() returns
 * logical block n.
 */
void ext2_set_block_follower(ext2_block_follower_t *bf, uint32_t n) {
    bf->next_block = n;
}
