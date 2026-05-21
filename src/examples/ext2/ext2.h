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

#pragma once

#include <ctype.h>

/*
 * ext2 related code. See the doc:
 *
 * https://www.nongnu.org/ext2-doc/ext2.pdf
 * 
 * for the gory details of the ext2 filesystem
 */


// Superblock
struct ext2_sb {
    uint32_t    total_inodes;
    uint32_t    total_blocks;
    uint32_t    total_reserved;
    uint32_t    num_free_blocks;
    uint32_t    num_free_inodes;
    uint32_t    sb_block_num;
    uint32_t    block_size;
    uint32_t    frag_size;
    uint32_t    blocks_per_block_group;
    uint32_t    frags_per_block_group;
    uint32_t    inodes_per_block_group;
    uint32_t    last_mount_time;
    uint32_t    last_written_time;
    uint16_t    mounts_since_checked;
    uint16_t    mounts_before_check;
    uint16_t    signature;
    uint16_t    fs_state;
    uint16_t    error_action;
    uint16_t    minor_version;
    uint32_t    last_checked;
    uint32_t    check_interval;
    uint32_t    creator_id;
    uint32_t    major_version;
    uint16_t    admin_uid;
    uint16_t    admin_group;

    /* 
     * Extended Superblock:
     * The following fields will only be present if the 
     * major version is 1 or higher 
     */

    uint32_t    first_free_inode;
    uint16_t    inode_size;
    uint16_t    block_group;
    uint32_t    optional_features;
    uint32_t    required_features;
    uint32_t    force_ro_features;
    uint8_t     blkid[16];
    uint8_t     volume_name[16];
    uint8_t     last_mounted_at[64];
    uint32_t    compression_algorithms;
    uint8_t     file_blocks_to_preallocate;
    uint8_t     dir_blocks_to_preallocate;
    uint16_t    unused_1;
    uint8_t     journal_id[16];
    uint32_t    journal_inode;
    uint32_t    journal_device;
    uint32_t    orphan_inode_head;
};

typedef struct ext2_sb ext2_sb_t;

// Block Group Descriptor
struct ext2_bg {
    uint32_t    bitmap_block_lba;
    uint32_t    inode_table_lba;
    uint16_t    num_free_blocks;
    uint16_t    num_free_inodes;
    uint16_t    num_directories;
    uint8_t     unused[18];
};

typedef struct ext2_bg ext2_bg_t;

struct ext2_inode {
    uint16_t    type_and_perms;
    uint16_t    user_id;
    uint32_t    lower_size;
    uint32_t    last_access_time;
    uint32_t    creation_time;
    uint32_t    last_modification_time;
    uint32_t    deletion_time;
    uint16_t    group_id;
    uint16_t    num_hard_links;
    uint32_t    num_sectors;                // Yes, sectors!
    uint32_t    flags;
    uint32_t    os_val;
    uint32_t    direct_blocks[12];
    uint32_t    single_indirect_pointer;
    uint32_t    double_indirect_pointer;
    uint32_t    triple_indirect_pointer;
    uint32_t    generation_number;
    uint32_t    extended_attribute_block;   // version >= 1, otherwise reserved.
    uint32_t    upper_size;                 // version >= 1, otherwise reserved.
    uint32_t    fragment_address;
    uint8_t     os_specific[12];
};

typedef struct ext2_inode ext2_inode_t;

struct ext2_fs {
    uint8_t     *block_buffer;
    uint32_t    block_num_in_buffer;
    uint8_t     block_in_buffer_valid;
    int         part_num;
    ext2_sb_t   *sb;
    ext2_bg_t   *bgt;
    uint32_t    num_blockgroups;
    uint32_t    block_size;
    uint8_t     sectors_per_block;
};

typedef struct ext2_fs ext2_fs_t;

ext2_bg_t *ext2_get_blockgroup_descriptor(ext2_fs_t *fs, uint32_t blockgroup_num, ext2_bg_t *bg);
int ext2_read_block(ext2_fs_t *fs, uint32_t block_num, uint8_t *buffer);
int ext2_read_fs_block(ext2_fs_t *fs, uint32_t block_num);
int ext2_get_inode(ext2_fs_t *fs, uint32_t inode_num, ext2_inode_t *inode);
ext2_fs_t *ext2_mount(uint8_t part_num);
void ext2_sanitize_superblock(ext2_sb_t *sb);
ext2_fs_t *ext2_umount(ext2_fs_t *fs);
int is_ext2(ext2_sb_t *sb);

void printn(const char *pfx, const uint8_t *str, int len);
void dump(uint8_t *buf, size_t count, uint8_t print_zeroes);
