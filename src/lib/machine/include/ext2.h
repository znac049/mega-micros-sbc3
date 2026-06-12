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

#define EXT2_SB_MAGIC 0xef53

#define EXT2_FT_UNKNOWN     0
#define EXT2_FT_REG_FILE    1
#define EXT2_FT_DIR         2
#define EXT2_FT_CHRDEV      3
#define EXT2_FT_BLKDEV      4
#define EXT2_FT_FIFO        5
#define EXT2_FT_SOCK        6
#define EXT2_FT_SYMLINK     7

// Reserved Inodes
#define EXT2_BAD_INO            1
#define EXT2_ROOT_INO           2
#define EXT2_ACL_IDX_INO        3       // ACL Index inode - possibly deprecated
#define EXT2_ACL_DATA_INO       4       // ACL: data inode - possibly deprecated
#define EXT2_BOOT_LOADER_INO    5       // Boot loader inode
#define EXT2_UNDEL_DIR_INO      6       // Undelete directory inode

// inode i_mode values
#define EXT2_S_IFSOCK   0xc000
#define EXT2_S_IFLNK    0xa000
#define EXT2_S_IFREG    0x8000
#define EXT2_S_IFBLK    0x6000
#define EXT2_S_IFDIR    0x4000
#define EXT2_S_IFCHR    0x2000
#define EXT2_S_IFIFO    0x1000

#define EXT2_S_ISUID    0x0800
#define EXT2_S_ISGID    0x0400
#define EXT2_S_ISVTX    0x0200

#define EXT2_S_IRUSR    0x0100
#define EXT2_S_IWUSR    0x0080
#define EXT2_S_IXUSR    0x0040
#define EXT2_S_IRGRP    0x0020
#define EXT2_S_IWGRP    0x0010
#define EXT2_S_IXGRP    0x0008
#define EXT2_S_IROTH    0x0004
#define EXT2_S_IWOTH    0x0002
#define EXT2_S_IXOTH    0x0001

#define EXT2_MAX_LEN        255
#define EXT2_MAX_DIR_DEPTH  16
#define EXT2_MAX_PATH_LEN   511

// Mode testing macros
#define S_ISREG(m) ((m&0xf000)==EXT2_S_IFREG)
#define S_ISDIR(m) ((m&0xf000)==EXT2_S_IFDIR)
#define S_ISCHR(m) ((m&0xf000)==EXT2_S_IFCHR)
#define S_ISBLK(m) ((m&0xf000)==EXT2_S_IFBLK)
#define S_ISFIFO(m) ((m&0xf000)==EXT2_S_IFIFO)
#define S_ISLNK(m) ((m&0xf000)==EXT2_S_IFLNK)
#define S_ISSOCK(m) ((m&0xf000)==EXT2_S_IFSOCK)


/*
 * ext2 related code. See the doc:
 *
 * https://www.nongnu.org/ext2-doc/ext2.pdf
 * 
 * for the gory details of the ext2 filesystem
 */


// Superblock
struct ext2_sb {
    uint32_t    s_inodes_count;         // total_inodes;
    uint32_t    s_blocks_count;         // total_blocks;
    uint32_t    s_r_blocks_count;       // total_reserved;
    uint32_t    s_free_blocks_count;    // num_free_blocks;
    uint32_t    s_free_inodes_count;    // num_free_inodes;
    uint32_t    s_first_data_block;     // sb_block_num;
    uint32_t    s_log_block_size;       // block_size;
    uint32_t    s_log_frag_size;        // frag_size;
    uint32_t    s_blocks_per_group;     // blocks_per_block_group;
    uint32_t    s_frags_per_group;      // frags_per_block_group;
    uint32_t    s_inodes_per_group;     // inodes_per_block_group;
    uint32_t    s_mtime;                // last_mount_time;
    uint32_t    s_wtime;                // last_written_time;
    uint16_t    s_mnt_count;            // mounts_since_checked;
    int16_t     s_max_mnt_count;        // mounts_before_check;
    uint16_t    s_magic;                // signature;
    uint16_t    s_state;                // fs_state;
    uint16_t    s_errors;               // error_action;
    uint16_t    s_minor_rev_level;      // minor_version;
    uint32_t    s_lastcheck;            // last_checked;
    uint32_t    s_checkinterval;        // check_interval;
    uint32_t    s_creator_os;           // creator_id;
    uint32_t    s_rev_level;            // major_version;
    uint16_t    s_def_resuid;           // admin_uid;
    uint16_t    s_def_resgid;           // admin_group;

    /* 
     * Extended Superblock:
     * The following fields will only be present if the 
     * major version is 1 or higher 
     */

    uint32_t    s_first_ino;            // first_free_inode;
    uint16_t    s_inode_size;           // inode_size;
    uint16_t    s_block_group_nr;       // block_group;
    uint32_t    s_feature_compat;       // optional_features;
    uint32_t    s_feature_incompat;     // required_features;
    uint32_t    s_feature_ro_compat;    // force_ro_features;
    uint8_t     s_uuid[16];             // blkid[16];
    uint8_t     s_volume_name[16];      // volume_name[16];
    uint8_t     s_last_mounted[64];     // last_mounted_at[64];
    uint32_t    s_algo_bitmap;          // compression_algorithms;
    uint8_t     s_prealloc_blocks;      // file_blocks_to_preallocate;
    uint8_t     s_prealloc_dir_blocks;  // dir_blocks_to_preallocate;
    uint16_t    unused;
    uint8_t     s_journal_uuid[16];     // journal_id[16];
    uint32_t    s_journal_inum;         // journal_inode;
    uint32_t    s_journal_dev;          // journal_device;
    uint32_t    s_last_orphan;          // orphan_inode_head;
    uint32_t    s_hash_seed[4];
    uint8_t     s_def_hash_version;
    uint8_t     padding[3];
    uint32_t    s_default_mount_options;
    uint32_t    s_first_meta_bg;
    uint8_t     reserved[760];
};

typedef struct ext2_sb ext2_sb_t;

// Block Group Descriptor
struct ext2_bg {
    uint32_t    bg_block_bitmap;
    uint32_t    bg_inode_bitmap;
    uint32_t    bg_inode_table;
    uint16_t    bg_free_blocks_count;
    uint16_t    bg_free_inodes_count;
    uint16_t    bg_used_dirs_count;
    uint16_t    bg_pad;
    uint8_t     bg_reserved[12];
};

typedef struct ext2_bg ext2_bg_t;

struct ext2_inode {
    uint16_t    i_mode;
    uint16_t    i_uid;
    uint32_t    i_size;
    uint32_t    i_atime;
    uint32_t    i_ctime;
    uint32_t    i_mtime;
    uint32_t    i_dtime;
    uint16_t    i_gid;
    uint16_t    i_links_count;
    uint32_t    i_blocks;
    uint32_t    i_flags;
    uint32_t    i_osdl;
    uint32_t    i_block[15];
    uint32_t    i_generation;
    uint32_t    i_file_acl;
    uint32_t    i_dir_acl;
    uint32_t    i_faddr;
    uint8_t     i_osd2[12];
};

typedef struct ext2_inode ext2_inode_t;

#define EXT2_SNGL_IND   12
#define EXT2_DBL_IND    13
#define EXT2_TRIP_IND   14

struct ext2_fs {
    uint8_t     *block_buffer;
    uint32_t    block_num_in_buffer;
    uint8_t     block_in_buffer_valid;
    int         part_num;
    ext2_sb_t   *sb;
    uint32_t    num_blockgroups;
    uint32_t    block_size;
    uint8_t     sectors_per_block;
    uint32_t    *bg_ent;
};

typedef struct ext2_fs ext2_fs_t;

struct ext2_dirent {
    uint32_t    inode;
    uint16_t    rec_len;
    uint8_t     name_len;
    uint8_t     file_type;
    uint8_t     name[EXT2_MAX_LEN];
};

typedef struct ext2_dirent ext2_dirent_t;

struct ext2_dir {
    ext2_inode_t    *inode;
    uint32_t        block_index;
    uint32_t        offset;
    ext2_dirent_t   dirent;
    ext2_fs_t       *fs;
};

typedef struct ext2_dir ext2_dir_t;

struct ext2_block_follower {
    ext2_fs_t       *fs;
    uint32_t        inode_num;
    ext2_inode_t    inode;
    uint32_t        direct_offset;
    uint32_t        single_offset;
    uint32_t        double_offset;
    uint32_t        triple_offset;
};

typedef struct ext2_block_follower ext2_block_follower_t;

// dir.c
int ext2_closedir(ext2_dir_t *dirp);
ext2_dir_t *ext2_opendir(ext2_fs_t *fs, const char *name);
ext2_dirent_t *ext2_readdir(ext2_dir_t *dirp);
void ext2_rewinddir(ext2_dir_t *dirp);

// dump.c
void dump_ext2_bg(ext2_bg_t *bg, int bg_num, ext2_sb_t *sb);
void dump_ext2_inode(ext2_inode_t *in, int in_num);
void dump_ext2_sb(ext2_sb_t *sb);

// endian.c
void ext2_sanitize_superblock(ext2_sb_t *src_sb, ext2_sb_t *dst_sb);
void ext2_sanitize_bg(ext2_bg_t *src_bg, ext2_bg_t *dst_bg);
void ext2_sanitize_inode(ext2_inode_t *src_in, ext2_inode_t *dst_in);
void ext2_sanitize_dirent(ext2_dirent_t *src_dp, ext2_dirent_t *dst_dp);

// ext2.c
ext2_bg_t *ext2_get_blockgroup_descriptor(ext2_fs_t *fs, uint32_t blockgroup_num, ext2_bg_t *bg);
int ext2_read_block(ext2_fs_t *fs, uint32_t block_num, uint8_t *buffer);
int ext2_read_fs_block(ext2_fs_t *fs, uint32_t block_num);
int ext2_get_inode(ext2_fs_t *fs, uint32_t inode_num, ext2_inode_t *inode);
ext2_fs_t *ext2_mount(uint8_t part_num);
ext2_fs_t *ext2_umount(ext2_fs_t *fs);
int is_ext2(ext2_sb_t *sb);

// file.c
int ext2_file_reader(ext2_fs_t *fs, uint32_t inode_num);

// utils.c
// void printn(const char *pfx, const uint8_t *str, int len);

