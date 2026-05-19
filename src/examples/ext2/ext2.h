#pragma once

#include <ctype.h>

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
