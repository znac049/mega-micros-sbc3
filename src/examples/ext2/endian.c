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
#include <machine.h>
#include <errno.h>
#include <string.h>

#include "ext2.h"
#include "disk.h"

void ext2_sanitize_superblock(ext2_sb_t *src_sb, ext2_sb_t *dst_sb) {
#if __BYTE_ORDER__ != __ORDER_BIG_ENDIAN__
    if (src_sb != dst_sb) {
        memcpy(dst_sb, src_sb, sizeof(ext2_sb_t));
    }
#else
    dst_sb->s_inodes_count = __builtin_bswap32(src_sb->s_inodes_count);
    dst_sb->s_blocks_count = __builtin_bswap32(src_sb->s_blocks_count);
    dst_sb->s_r_blocks_count = __builtin_bswap32(src_sb->s_r_blocks_count);
    dst_sb->s_free_blocks_count = __builtin_bswap32(src_sb->s_free_blocks_count);
    dst_sb->s_free_inodes_count = __builtin_bswap32(src_sb->s_free_inodes_count);
    dst_sb->s_first_data_block = __builtin_bswap32(src_sb->s_first_data_block);
    dst_sb->s_log_block_size = __builtin_bswap32(src_sb->s_log_block_size);
    dst_sb->s_log_frag_size = __builtin_bswap32(src_sb->s_log_frag_size);
    dst_sb->s_blocks_per_group = __builtin_bswap32(src_sb->s_blocks_per_group);
    dst_sb->s_frags_per_group = __builtin_bswap32(src_sb->s_frags_per_group);
    dst_sb->s_inodes_per_group = __builtin_bswap32(src_sb->s_inodes_per_group);
    dst_sb->s_mtime = __builtin_bswap32(src_sb->s_mtime);
    dst_sb->s_wtime = __builtin_bswap32(src_sb->s_wtime);
    dst_sb->s_mnt_count = __builtin_bswap16(src_sb->s_mnt_count);
    dst_sb->s_max_mnt_count = __builtin_bswap16(src_sb->s_max_mnt_count);
    dst_sb->s_magic = __builtin_bswap16(src_sb->s_magic);
    dst_sb->s_state = __builtin_bswap16(src_sb->s_state);
    dst_sb->s_errors = __builtin_bswap16(src_sb->s_errors);
    dst_sb->s_minor_rev_level = __builtin_bswap16(src_sb->s_minor_rev_level);
    dst_sb->s_lastcheck = __builtin_bswap32(src_sb->s_lastcheck);
    dst_sb->s_checkinterval = __builtin_bswap32(src_sb->s_checkinterval);
    dst_sb->s_creator_os = __builtin_bswap32(src_sb->s_creator_os);
    dst_sb->s_rev_level = __builtin_bswap32(src_sb->s_rev_level);
    dst_sb->s_def_resuid = __builtin_bswap16(src_sb->s_def_resuid);
    dst_sb->s_def_resgid = __builtin_bswap16(src_sb->s_def_resgid);

    if (src_sb->s_rev_level >= 1) {
        dst_sb->s_first_ino = __builtin_bswap32(src_sb->s_first_ino);
        dst_sb->s_inode_size = __builtin_bswap16(src_sb->s_inode_size);
        dst_sb->s_block_group_nr = __builtin_bswap16(src_sb->s_block_group_nr);
        dst_sb->s_feature_compat = __builtin_bswap32(src_sb->s_feature_compat);
        dst_sb->s_feature_incompat = __builtin_bswap32(src_sb->s_feature_incompat);
        dst_sb->s_feature_ro_compat = __builtin_bswap32(src_sb->s_feature_ro_compat);
        dst_sb->s_algo_bitmap = __builtin_bswap32(src_sb->s_algo_bitmap);
        dst_sb->s_journal_inum = __builtin_bswap32(src_sb->s_journal_inum);
        dst_sb->s_journal_dev = __builtin_bswap32(src_sb->s_journal_dev);
        dst_sb->s_last_orphan = __builtin_bswap32(src_sb->s_last_orphan);
        dst_sb->s_hash_seed[0] = __builtin_bswap32(src_sb->s_hash_seed[0]);
        dst_sb->s_hash_seed[1] = __builtin_bswap32(src_sb->s_hash_seed[1]);
        dst_sb->s_hash_seed[2] = __builtin_bswap32(src_sb->s_hash_seed[2]);
        dst_sb->s_hash_seed[3] = __builtin_bswap32(src_sb->s_hash_seed[3]);
        dst_sb->s_default_mount_options = __builtin_bswap32(src_sb->s_default_mount_options);
        dst_sb->s_first_meta_bg = __builtin_bswap32(src_sb->s_first_meta_bg);

        strcpy((char *)dst_sb->s_volume_name, (char *)src_sb->s_volume_name);
        memcpy(dst_sb->s_uuid, src_sb->s_uuid, sizeof(dst_sb->s_uuid));
        strcpy((char *)dst_sb->s_last_mounted, (char *)src_sb->s_last_mounted);
    }
    else {
        dst_sb->s_first_ino = 0;
        dst_sb->s_inode_size = 128;
        dst_sb->s_block_group_nr = 0;
        dst_sb->s_feature_compat = 0;
        dst_sb->s_feature_incompat = 0;
        dst_sb->s_feature_ro_compat = 0;
        dst_sb->s_algo_bitmap = 0;
        dst_sb->s_journal_inum = 0;
        dst_sb->s_journal_dev = 0;
        dst_sb->s_last_orphan = 0;
        dst_sb->s_hash_seed[0] = 0;
        dst_sb->s_hash_seed[1] = 0;
        dst_sb->s_hash_seed[2] = 0;
        dst_sb->s_hash_seed[3] = 0;
        dst_sb->s_def_hash_version = 0;
        dst_sb->s_default_mount_options = 0;
        dst_sb->s_first_meta_bg = 0;

        strcpy((char *)dst_sb->s_volume_name, "<none>");
        memset(dst_sb->s_uuid, 0, sizeof(dst_sb->s_uuid));
        strcpy((char *)dst_sb->s_last_mounted, "<none>");
    }
#endif
}

void ext2_sanitize_bg(ext2_bg_t *src_bg, ext2_bg_t *dst_bg) {
#if __BYTE_ORDER__ != __ORDER_BIG_ENDIAN__
    if (src_bg != dst_bg) {
        memcpy(dst_bg, src_bg, sizeof(ext2_bg_t));
    }
#else
    dst_bg->bg_block_bitmap = __builtin_bswap32(src_bg->bg_block_bitmap);
    dst_bg->bg_inode_bitmap = __builtin_bswap32(src_bg->bg_inode_bitmap);
    dst_bg->bg_inode_table = __builtin_bswap32(src_bg->bg_inode_table);
    dst_bg->bg_free_blocks_count = __builtin_bswap16(src_bg->bg_free_blocks_count);
    dst_bg->bg_free_inodes_count = __builtin_bswap16(src_bg->bg_free_inodes_count);
    dst_bg->bg_used_dirs_count = __builtin_bswap16(src_bg->bg_used_dirs_count);
#endif
}

void ext2_sanitize_inode(ext2_inode_t *src_in, ext2_inode_t *dst_in) {
#if __BYTE_ORDER__ != __ORDER_BIG_ENDIAN__
    if (src_in != dst_in) {
        memcpy(dst_in, src_in, sizeof(ext2_inode_t));
    }
#else
    dst_in->i_mode = __builtin_bswap16(src_in->i_mode);
    dst_in->i_uid = __builtin_bswap16(src_in->i_uid);
    dst_in->i_size = __builtin_bswap32(src_in->i_size);
    dst_in->i_atime = __builtin_bswap32(src_in->i_atime);
    dst_in->i_ctime = __builtin_bswap32(src_in->i_ctime);
    dst_in->i_mtime = __builtin_bswap32(src_in->i_mtime);
    dst_in->i_dtime = __builtin_bswap32(src_in->i_dtime);
    dst_in->i_gid = __builtin_bswap16(src_in->i_gid);
    dst_in->i_links_count = __builtin_bswap16(src_in->i_links_count);
    dst_in->i_blocks = __builtin_bswap32(src_in->i_blocks);
    dst_in->i_flags = __builtin_bswap32(src_in->i_flags);
    dst_in->i_osdl = __builtin_bswap32(src_in->i_osdl);

    for (int i=0; i<15; i++) {
        dst_in->i_block[i] = __builtin_bswap32(src_in->i_block[i]);
    }

    dst_in->i_generation = __builtin_bswap32(src_in->i_generation);
    dst_in->i_file_acl = __builtin_bswap32(src_in->i_file_acl);
    dst_in->i_dir_acl = __builtin_bswap32(src_in->i_dir_acl);
    dst_in->i_faddr = __builtin_bswap32(src_in->i_faddr);
#endif
}
