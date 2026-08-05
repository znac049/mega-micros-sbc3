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
#include <time.h>
#include <ext2.h>
#include <disk.h>

#if defined(BAREMETAL)

struct fs_feature {
    int mask;
    char *feature;
};

typedef struct fs_feature fs_feature_t;

void dump_ext2_sb(ext2_sb_t *sb) {
    uint32_t bg1;
    uint32_t bg2;
    uint32_t inodes_per_block = (1024<<sb->s_log_block_size) / sb->s_inode_size;

    fs_feature_t compat_features[] = {
        {0x0001, "dir_prealloc"},
        {0x0002, "magic_inodes"},
        {0x0004, "ext3_journal"},
        {0x0008, "ext_attr"},
        {0x0010, "resize_inode"},
        {0x0020, "dir_index"},
        {0, ""}
    };

    fs_feature_t incompat_features[] = {
        {0x0001, "compression"},
        {0x0002, "filetype"},
        {0x0004, "recover"},
        {0x0008, "journal_dev"},
        {0x0010, "meta_bg"},
        {0, ""}
    };

    fs_feature_t ro_features[] = {
        {0x0001, "sparse_super"},
        {0x0002, "large_file"},
        {0x0004, "btree_dir"},
        {0, ""}
    };

    char *os_types[] = {"Linux", "GNU Hurd", "MASIX", "FreeBSD", "Lites"};

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

    printf("Superblock:\n");
    printf("  Filesystem volume name:  %s\n", sb->s_volume_name);
    printf("  Last mounted as:         %s\n", sb->s_last_mounted);
    printf("  Filesystem UUID:\n");
    printf("  Filesystem magic number: 0x%04x\n", sb->s_magic);
    printf("  Filesystem revision #:   %d.%d\n", sb->s_rev_level, sb->s_minor_rev_level);
    printf("  Filesystem features:     ", sb->s_feature_compat);
    for (int i=0; compat_features[i].mask; i++) {
        if (sb->s_feature_compat & compat_features[i].mask) {
            printf("%s(c) ", compat_features[i].feature);
        }
    }

    for (int i=0; incompat_features[i].mask; i++) {
        if (sb->s_feature_incompat & incompat_features[i].mask) {
            printf("%s(i) ", incompat_features[i].feature);
        }
    }

    for (int i=0; ro_features[i].mask; i++) {
        if (sb->s_feature_ro_compat & ro_features[i].mask) {
            printf("%s(ro) ", ro_features[i].feature);
        }
    }
    printf("\n");
    
    printf("  Filesystem state:        ");
    switch(sb->s_state) {
        case 1:     printf("clean\n"); break;
        case 2:     printf("errors\n"); break;
    }

    printf("  Errors behaviour:        ");
    switch(sb->s_errors) {
        case 1:     printf("Continue\n"); break;
        case 2:     printf("Remount RO\n"); break;
        case 3:     printf("Panic\n"); break;
    }

    printf("  Filesystem OS type:      %s\n", (sb->s_creator_os <= 4)?os_types[sb->s_creator_os]:"<unknown>");

    printf("  Inode count:             %d\n", sb->s_inodes_count);
    printf("  Block count:             %d\n", sb->s_blocks_count);
    printf("  Reserved block count:    %d\n", sb->s_r_blocks_count);
    printf("  Overhead clusters:       ????\n");
    printf("  Free blocks:             %d\n", sb->s_free_blocks_count);
    printf("  Free inodes:             %d\n", sb->s_free_inodes_count);
    printf("  First block:             %d\n", sb->s_first_data_block);
    printf("  Block Size:              %d\n", 1024<<sb->s_log_block_size);
    printf("  Fragment size:           %d\n", 1024<<sb->s_log_frag_size);
    printf("  Reserved GDT blocks:     ????\n");
    printf("  Blocks per group:        %d\n", sb->s_blocks_per_group);
    printf("  Fragments per group:     %d\n", sb->s_frags_per_group);
    printf("  Inodes per group:        %d\n", sb->s_inodes_per_group);
    printf("  Inode blocks per group   %d\n", sb->s_inodes_per_group / inodes_per_block);
    printf("  Last mount time:         %s", ctime(&sb->s_mtime));
    printf("  Last write time:         %s", ctime(&sb->s_wtime));
    printf("  Mount count:             %d\n", sb->s_mnt_count);
    printf("  Maximum mount count:     %d\n", sb->s_max_mnt_count);
    printf("  Last checked:            %s\n", ctime(&sb->s_lastcheck));
    printf("  Check interval:          %d\n", sb->s_checkinterval);
    printf("  Inode size:              %d\n", sb->s_inode_size);
}

void dump_ext2_bg(ext2_bg_t *bg, int bg_num, ext2_sb_t *sb) {
    uint32_t inodes_per_block = (1024<<sb->s_log_block_size) / sb->s_inode_size;
    uint32_t num_inode_blocks = sb->s_inodes_per_group / inodes_per_block;
    uint32_t first_block = bg_num * sb->s_blocks_per_group;

    printf("\nBlock Group %d: (Blocks %d-%d\n", bg_num, bg_num * sb->s_blocks_per_group, ((bg_num + 1) * sb->s_blocks_per_group) - 1);
    if (ext2_has_superblock(bg_num) == YES) {
        printf("  %s superblock at %d, Group descriptors at %d-%d\n", (bg_num == 0)?"Primary":"Backup", first_block, first_block+1, first_block+1);  // TODO: Fix end of bgdt
    }
    printf("  Block Bitmap at %d (+%d)\n", bg->bg_block_bitmap, bg->bg_block_bitmap - first_block);
    printf("  Inode Bitmap at %d (+%d)\n", bg->bg_inode_bitmap, bg->bg_inode_bitmap - first_block);
    printf("  Inode Table at %d-%d (+%d)\n", bg->bg_inode_table, bg->bg_inode_table + num_inode_blocks - 1, bg->bg_inode_table - first_block);
    printf("  %d free blocks, %d free inodes, %d directories\n", 
        bg->bg_free_blocks_count, bg->bg_free_inodes_count, bg->bg_used_dirs_count);   
}

void dump_ext2_inode(ext2_inode_t *in, int in_num) {
    printf("Inode %d:\n", in_num);
    printf("  Mode:              %04x\n", in->i_mode);
    printf("  User/Group Id:     %d:%d\n", in->i_uid, in->i_gid);
    printf("  Size:              %d\n", in->i_size);
    printf("  Last access:       %s", ctime(&in->i_atime));
    printf("  Created:           %s", ctime(&in->i_ctime));
    printf("  Modified:          %s", ctime(&in->i_mtime));
    printf("  Deleted:           %s", ctime(&in->i_dtime));
    printf("  # Links:           %d\n", in->i_links_count);
    printf("  # Blocks:          %d\n", in->i_blocks);
    printf("  Flags:             %08x\n", in->i_flags);
    printf("  OS val:            %08x\n", in->i_osdl);

    printf("  Blocks:            ");
    for (int i=0; i<15; i++) {
        switch (i) {
            case EXT2_SNGL_IND:
                printf("[%d], ", in->i_block[i]);
                break;

            case EXT2_DBL_IND:
                printf("[[%d]], ", in->i_block[i]);
                break;

            case EXT2_TRIP_IND:
                printf("[[[%d]]]\n", in->i_block[i]);
                break;

            default:
                printf("%d, ", in->i_block[i]);
                break;
        }
    }

    printf("  Generation #:      %d\n", in->i_generation);
}

#endif