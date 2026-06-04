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
#include <malloc.h>

#include "ext2.h"
#include "disk.h"

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
    int start_sector = (block_num - 1) * fs->sectors_per_block;

    printf("ext2_read_block %d, (sector %d)\n", block_num, start_sector);

    for (int i=0; i< fs->sectors_per_block; i++) {
        int sec = partition_read(fs->part_num, start_sector+i, &buffer[i * CF_SECTOR_SIZE]);

        sec++;
    }

    return 0;
}

static int ext2_read_superblock(uint8_t part_num, ext2_sb_t *sb) {
    uint8_t buf[CF_SECTOR_SIZE];

    if (partition_read(part_num, 2, buf) != 0) {
        return -1;
    }

    ext2_sanitize_superblock((ext2_sb_t *)buf, sb);

    return 0;
}

int is_ext2(ext2_sb_t *sb) {
    if (sb->s_magic != 0xef53) {
        return 0;
    }

    return 1;
}

int ext2_get_bg(ext2_fs_t *fs, uint32_t bg_num, ext2_bg_t *bg) {
    uint32_t ents_per_block = fs->block_size / sizeof(ext2_bg_t);
    uint32_t block_num = bg_num / ents_per_block;
    uint32_t bg_index = bg_num % ents_per_block;
    ext2_bg_t *bgs = (ext2_bg_t *) fs->block_buffer;

    block_num += (fs->block_size == 1024)?3:2;

    printf("\n\n++++++++++\next2_get_bg(?, %d,...)\n", bg_num);
    printf("  -> block %d, index %d (%d entries per block)\n", block_num, bg_index, ents_per_block);

    // Grab the block with the entry
    if (ext2_read_fs_block(fs, block_num) != 0) {
        printf("Failed to read ext2 block %d\n", block_num);
        return -1;
    }

    ext2_sanitize_bg(&bgs[bg_index], bg);
    dump_ext2_bg(bg, bg_num, fs->sb);
    
    return 0;
}

int ext2_get_inode(ext2_fs_t *fs, uint32_t inode_num, ext2_inode_t *inode) {
    uint32_t block_group_num = (inode_num - 1) / fs->sb->s_inodes_per_group;
    uint32_t block_group_base = block_group_num * fs->sb->s_blocks_per_group;
    uint32_t index = (inode_num - 1) % fs->sb->s_inodes_per_group;
    uint32_t block_num = (index * fs->sb->s_inode_size) / fs->block_size;
    ext2_inode_t *ent;
    ext2_bg_t bg;
    uint32_t offset = index * fs->sb->s_inode_size;;

    if (ext2_get_bg(fs, block_group_num, &bg) != 0) {
        printf("Failed to get bg %d\n", block_group_num);
        return -1;
    }

    printf("First data block is %d\n", fs->sb->s_first_data_block);

    // printf("in get_inode: inode %d is in block group %d.\n", inode_num, block_group);
    // printf("block size is %d\n", fs->block_size);

    // printf("Block Group Table entry %d:\n", block_group);
    // printf("  inode table LBA: %d\n", bg.bg_inode_table);
    printf("\nindex=%d, offset=%d, block_num=%d, inode_size=%d (%d)\n\n", index, offset, block_num, fs->sb->s_inode_size, sizeof(ext2_inode_t));
    printf("inode block is %d + %d (%d)\n", bg.bg_inode_table, block_num, bg.bg_inode_table + block_num);
    if (ext2_read_fs_block(fs, block_group_base + bg.bg_inode_table + block_num) != 0) {
        return -1;
    }

    ent = (ext2_inode_t *)&fs->block_buffer[offset];

    ext2_sanitize_inode(ent, inode);
    dump_ext2_inode(inode, inode_num);
    
    return 0;
}

ext2_fs_t *ext2_mount(uint8_t part_num) {
    ext2_sb_t *sb;
    int res;
    uint32_t bg1;
    uint32_t bg2;
    ext2_fs_t *fs = NULL;
    uint8_t *buffer = NULL;
    uint32_t block_size;

    sb = malloc(sizeof(ext2_sb_t));
    if (sb == NULL) {
        return NULL;
    }

    res = ext2_read_superblock(part_num, sb);
    if (res != 0) {
        free(sb);
        return NULL;
    }

    if (!is_ext2(sb)) {
        printf("Not an ext2 filesystem!\n");
        free(sb);
        return NULL;
    }

    dump_ext2_sb(sb);

    // Calculate the number of block groups two different ways and check both
    // answers are the same.
    bg1 = sb->s_blocks_count / sb->s_blocks_per_group;
    if ((bg1 * sb->s_blocks_per_group) < sb->s_blocks_count) {
        bg1++;
    }

    bg2 = sb->s_inodes_count / sb->s_inodes_per_group;
    if ((bg2 * sb->s_inodes_per_group) < sb->s_inodes_count) {
        bg2++;
    }

    if (bg1 != bg2) {
        printf("Number of Block groups calculations inconsistency: %d != %d.\n", bg1, bg2);
        free(sb);
        return NULL;
    }

    block_size = 1024<<sb->s_log_block_size;

    // printf("bg table entries=%d, entry size=%d\n", bg1, sizeof(ext2_bg_t));
    // printf("bg table entries per block=%d\n", block_size / sizeof(ext2_bg_t));

    fs = malloc(sizeof(ext2_fs_t));
    buffer = malloc(block_size);

    if ((fs == NULL) || (buffer == NULL)) {
        printf("Out of memory.\n");
        free(fs);
        free(buffer);
        free(sb);
        return NULL;
    }

    printf("ext2_mount: all ok.\n");
    // All good !
    fs->part_num = part_num;
    fs->sb = sb;
    fs->num_blockgroups = bg1;
    fs->block_size = block_size;
    fs->block_buffer = buffer;
    fs->block_num_in_buffer = 0;
    fs->block_in_buffer_valid = 0;
    fs->sectors_per_block = fs->block_size / CF_SECTOR_SIZE;

    return fs;
}

ext2_fs_t *ext2_umount(ext2_fs_t *fs) {
    printf("unmount ext2 filesystem on partition %d.\n", fs->part_num);

    free(fs->sb);
    free(fs->block_buffer);
    free(fs);

    return NULL;
}