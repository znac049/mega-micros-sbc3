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

int ext2_read_fs_block(ext2_fs_t *fs, uint32_t block_num) {
    int res;
    
    // Do we already have that block?
    if ((fs->block_num_in_buffer == block_num) &&
        (fs->block_in_buffer_valid == 1)) {
            // printf("block %d already in the buffer\n", block_num);
            return 0;
    }

    res = ext2_read_block(fs, block_num, fs->block_buffer);
    
    if (res == 0) {
        fs->block_num_in_buffer = block_num;
        fs->block_in_buffer_valid = 1;
    }
    else {
        fs->block_in_buffer_valid = 0;
    }

    return res;
}

int ext2_read_block(ext2_fs_t *fs, uint32_t block_num, uint8_t *buffer) {
    int start_sector = (block_num) * fs->sectors_per_block;

    // printf("ext2_read_block %d, (sector %d)\n", block_num, start_sector);

    for (int i=0; i< fs->sectors_per_block; i++) {
        int sec = partition_read(fs->part_num, start_sector+i, &buffer[i * CF_SECTOR_SIZE]);

        sec++;
    }

    return 0;
}

uint32_t ext2_get_first_block_num(ext2_fs_t *fs, uint32_t inode_num, ext2_block_follower_t *bf) {
    bf->fs = fs;
    bf->inode_num = inode_num;

    if (ext2_get_inode(fs, inode_num, &bf->inode) != 0) {
        printf("Panic!!!!\n");
        return 0;
    }

    bf->direct_offset = 1;
    bf->single_offset = 0;
    bf->double_offset = 0;
    bf->triple_offset = 0;

    return bf->inode.i_block[0];
}

uint32_t ext2_get_next_block_num(ext2_block_follower_t *bf) {
    ext2_inode_t *in = &bf->inode;
    uint32_t block_num = 0;
    uint32_t indexes_per_block = bf->fs->block_size / sizeof(uint32_t);

    if (bf->direct_offset > EXT2_TRIP_IND) {
        return 0;
    }

    if (bf->direct_offset < EXT2_SNGL_IND) {
        block_num = in->i_block[bf->direct_offset++];
    }
    else if (bf->direct_offset == EXT2_SNGL_IND) {
        // Grab a copy of the single indirect block - remember to deal with
        // endianness.
        uint32_t    *bp = (uint32_t *)bf->fs->block_buffer;

        if (ext2_read_fs_block(bf->fs, in->i_block[EXT2_SNGL_IND]) != 0) {
            return 0;
        }

        block_num = __builtin_bswap32(bp[bf->single_offset++]);

        if (bf->single_offset >= indexes_per_block) {
            bf->direct_offset++;
            bf->single_offset = 0;
        }
    }
    else if (bf->direct_offset == EXT2_DBL_IND) {
        ;
    }
    else if (bf->direct_offset == EXT2_TRIP_IND) {
        ;
    }
    else {
        return 0;
    }

    return block_num;
}