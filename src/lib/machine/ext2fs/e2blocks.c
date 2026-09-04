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
#include <ext2.h>
#include <disk.h>

#if defined(BAREMETAL)

int ext2_read_block(vmp_t *mp, uint32_t block_num, uint8_t *buffer) {
    kprintf("ext2_read_block %d, device='%s%d'\n", block_num, mp->dev_driver->name, mp->subdev);

    return bd_read(mp->dev_driver, block_num, buffer, mp->subdev);
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
    kprintf("\nBlock Fololower (0x%08x):\n", bf);

    if (bf == NULL) {
        return;
    }

    kprintf("  inode_num: %d\n", bf->inode_num);
    kprintf("  inode: 0x%08x\n", bf->inode);
    kprintf("  direct_offset: %d\n", bf->direct_offset);
    kprintf("  single_offset: %d\n", bf->single_offset);
    kprintf("  double_offset: %d\n", bf->double_offset);
    kprintf("  triple_offset: %d\n", bf->triple_offset);
} 

void ext2_reset_block_follower(ext2_block_follower_t *bf) {
    bf->direct_offset = 0;
    bf->single_offset = 0;
    bf->double_offset = 0;
    bf->triple_offset = 0;
}

int ext2_init_block_follower(ext2_block_follower_t *bf, vmp_t *mp, uint32_t inode_num) {
    bf->inode_num = inode_num;
    bf->mp = mp;

    ext2_reset_block_follower(bf);

    // kprintf("ext2_init_block_follower: get inode %d\n", inode_num);
    if (ext2_get_inode(bf->mp, inode_num, &bf->inode) != 0) {
        return NOT_OK;
    }

    return OK;
}

uint32_t ext2_get_next_block_num(ext2_block_follower_t *bf) {
    ext2_inode_t *in = &bf->inode;
    uint32_t block_num = 0;
    uint32_t indexes_per_block = BLOCK_DEVICE_BLOCK_SIZE / sizeof(uint32_t);

    if (bf->direct_offset > EXT2_TRIP_IND) {
        return 0;
    }

    if (bf->direct_offset < EXT2_SNGL_IND) {
        block_num = in->i_block[bf->direct_offset++];
    }
    else if (bf->direct_offset == EXT2_SNGL_IND) {
        // Grab a copy of the single indirect block - remember to deal with
        // endianness.
        uint32_t *bp = (uint32_t *)bf->mp->block_buffer;

        if (ext2_read_fs_block(bf->mp, in->i_block[EXT2_SNGL_IND], YES) != 0) {
            return 0;
        }

        block_num = __builtin_bswap32(bp[bf->single_offset++]);

        if (bf->single_offset >= indexes_per_block) {
            bf->direct_offset++;
            bf->single_offset = 0;
        }
    }
    else if (bf->direct_offset == EXT2_DBL_IND) {
        kprintf("BLOCK FOLLOWER DOUBLE PROBLEM!\n");
        return 0;
    }
    else if (bf->direct_offset == EXT2_TRIP_IND) {
        kprintf("BLOCK FOLLOWER TRIPLE PROBLEM!\n");
        return 0;
    }
    else {
        return 0;
    }

    return block_num;
}

#endif // BAREMETAL