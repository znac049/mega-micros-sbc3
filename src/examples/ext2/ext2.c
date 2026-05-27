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
#include <malloc.h>

#include "ext2.h"
#include "disk.h"

int ext2_read_fs_block(ext2_fs_t *fs, uint32_t block_num) {
    int res;
    
    // Do we already have that block?
    if ((fs->block_num_in_buffer == block_num) &&
        (fs->block_in_buffer_valid == 1)) {
            printf("block %d already in the buffer\n", block_num);
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
    int start_sector = block_num * fs->sectors_per_block;

    printf("ext2_read_block %d, (sector %d)\n", block_num, start_sector);

    for (int i=0; i< fs->sectors_per_block; i++) {
        int sec = partition_read(fs->part_num, start_sector+i, &buffer[i * CF_SECTOR_SIZE]);

        sec++;
    }

    dump(buffer, fs->block_size, 0);

    return 0;
}

void ext2_sanitize_superblock(ext2_sb_t *sb) {
    // Deal with endianness

    sb->total_inodes = __builtin_bswap32(sb->total_inodes);
    sb->total_blocks = __builtin_bswap32(sb->total_blocks);
    sb->total_reserved = __builtin_bswap32(sb->total_reserved);
    sb->num_free_blocks = __builtin_bswap32(sb->num_free_blocks);
    sb->num_free_inodes = __builtin_bswap32(sb->num_free_inodes);
    sb->sb_block_num = __builtin_bswap32(sb->sb_block_num);
    sb->block_size = __builtin_bswap32(sb->block_size);
    sb->frag_size = __builtin_bswap32(sb->frag_size);
    sb->blocks_per_block_group = __builtin_bswap32(sb->blocks_per_block_group);
    sb->frags_per_block_group = __builtin_bswap32(sb->frags_per_block_group);
    sb->inodes_per_block_group = __builtin_bswap32(sb->inodes_per_block_group);
    sb->last_mount_time = __builtin_bswap32(sb->last_mount_time);
    sb->last_written_time = __builtin_bswap32(sb->last_written_time);
    sb->mounts_since_checked = __builtin_bswap16(sb->mounts_since_checked);
    sb->mounts_before_check = __builtin_bswap16(sb->mounts_before_check);
    sb->signature = __builtin_bswap16(sb->signature);
    sb->fs_state = __builtin_bswap16(sb->fs_state);
    sb->error_action = __builtin_bswap16(sb->error_action);
    sb->minor_version = __builtin_bswap16(sb->minor_version);
    sb->last_checked = __builtin_bswap32(sb->last_checked);
    sb->check_interval = __builtin_bswap32(sb->check_interval);
    sb->creator_id = __builtin_bswap32(sb->creator_id);
    sb->major_version = __builtin_bswap32(sb->major_version);
    sb->admin_uid = __builtin_bswap16(sb->admin_uid);
    sb->admin_group = __builtin_bswap16(sb->admin_group);

    if (sb->major_version) {
        sb->first_free_inode = __builtin_bswap32(sb->first_free_inode);
        sb->inode_size = __builtin_bswap16(sb->inode_size);
        sb->block_group = __builtin_bswap16(sb->block_group);
        sb->optional_features = __builtin_bswap32(sb->optional_features);
        sb->required_features = __builtin_bswap32(sb->required_features);
        sb->force_ro_features = __builtin_bswap32(sb->force_ro_features);
        sb->compression_algorithms = __builtin_bswap32(sb->compression_algorithms);
        sb->journal_inode = __builtin_bswap32(sb->journal_inode);
        sb->journal_device = __builtin_bswap32(sb->journal_device);
        sb->orphan_inode_head = __builtin_bswap32(sb->orphan_inode_head);
    }
    else {
        sb->first_free_inode = 0;
        sb->inode_size = 128;
        sb->block_group = 0;
        sb->optional_features = 0;
        sb->required_features = 0;
        sb->force_ro_features = 0;
        sb->compression_algorithms = 0;
        sb->journal_inode = 0;
        sb->journal_device = 0;
        sb->orphan_inode_head = 0;
    }
}

static int ext2_read_superblock(uint8_t part_num, ext2_sb_t *sb) {
    uint8_t buf[CF_SECTOR_SIZE];

    if (partition_read(part_num, 2, buf) != 0) {
        return -1;
    }

    memcpy(sb, buf, sizeof(ext2_sb_t));
    ext2_sanitize_superblock(sb);

    return 0;
}

int is_ext2(ext2_sb_t *sb) {
    if (sb->signature != 0xef53) {
        // printf("ext2 signature not found in superblock.\n");
        return 0;
    }

#if 0
    printf("Root superblock:\n");
    printf("        Signature: %04x\n", sb->signature);
    printf("          Version: %d.%d\n", sb->major_version, sb->minor_version);
    printf("     Total Blocks: %d\n", sb->total_blocks);
    printf("     Total Inodes: %d\n", sb->total_inodes);
    printf("       Block Size: %d\n", 1024<<sb->block_size);
    printf(" Blocks Per Group: %d\n", sb->blocks_per_block_group);
    printf(" Inodes Per Group: %d\n", sb->inodes_per_block_group);
    printf("     Number of Block Groups(B) -> %d\n", bg1);
    printf("     Number of Block Groups(I) -> %d\n", bg2);

    if (sb->major_version >= 1) {
        // Extended superblock will be present
        printn("     Volume Name: ", sb->volume_name, sizeof(sb->volume_name));
        printn(" Last Mounted As: ", sb->last_mounted_at, sizeof(sb->last_mounted_at));
        // printn("        Block Id: ", sb->blkid, sizeof(sb->blkid));
        printn("      Journal Id: ", sb->journal_id, sizeof(sb->journal_id));
    }
#endif

    return 1;
}

int ext2_get_inode(ext2_fs_t *fs, uint32_t inode_num, ext2_inode_t *inode) {
    uint32_t block_group = (inode_num -1) / fs->sb->inodes_per_block_group;
    uint32_t index = (inode_num - 1) % fs->sb->inodes_per_block_group;
    uint32_t block_num = (index * fs->sb->inode_size) / fs->block_size;
    ext2_inode_t *inodes = (ext2_inode_t *)fs->block_buffer;
    ext2_inode_t *ent;

    ext2_bg_t *bg = &fs->bgt[block_group];

    printf("in get_inode: inode %d is in block group %d.\n", inode_num, block_group);
    printf("block size is %d\n", fs->block_size);

    printf("Block Group Table entry %d:\n", block_group);
    printf("       bitmap LBA: %d\n", bg->bitmap_block_lba);
    printf("  inode table LBA: %d\n", bg->inode_table_lba);
    printf("    # free blocks: %d\n", bg->num_free_blocks);
    printf("    # free inodes: %d\n", bg->num_free_inodes);
    printf("    # directories: %d\n", bg->num_directories);

    printf("\nindex=%d, block_num=%d, inode_size=%d\n\n", index, block_num, fs->sb->inode_size);
    if (ext2_read_fs_block(fs, bg->inode_table_lba + block_num) != 0) {
        return -1;
    }

    ent = &inodes[index];

    inode->type_and_perms = __builtin_bswap16(ent->type_and_perms);
    inode->user_id = __builtin_bswap16(ent->user_id);
    inode->lower_size = __builtin_bswap32(ent->lower_size);
    inode->last_access_time = __builtin_bswap32(ent->last_access_time);
    inode->creation_time = __builtin_bswap32(ent->creation_time);
    inode->last_modification_time = __builtin_bswap32(ent->last_modification_time);
    inode->deletion_time = __builtin_bswap32(ent->deletion_time);
    inode->group_id = __builtin_bswap16(ent->group_id);
    inode->num_hard_links = __builtin_bswap16(ent->num_hard_links);
    inode->num_sectors = __builtin_bswap32(ent->num_sectors);
    inode->flags = __builtin_bswap32(ent->flags);
    inode->os_val = __builtin_bswap32(ent->os_val);

    for (int i=0; i<12; i++) {
        inode->direct_blocks[i] = __builtin_bswap32(ent->direct_blocks[i]);
    }

    inode->single_indirect_pointer = __builtin_bswap32(ent->single_indirect_pointer);
    inode->double_indirect_pointer = __builtin_bswap32(ent->double_indirect_pointer);
    inode->triple_indirect_pointer = __builtin_bswap32(ent->triple_indirect_pointer);
    inode->generation_number = __builtin_bswap32(ent->generation_number);
    inode->extended_attribute_block = __builtin_bswap32(ent->extended_attribute_block);
    inode->upper_size = __builtin_bswap32(ent->upper_size);
    inode->fragment_address = __builtin_bswap32(ent->fragment_address);
    
    return 0;
}

ext2_bg_t *ext2_get_blockgroup_descriptor(ext2_fs_t *fs, uint32_t blockgroup_num, ext2_bg_t *bg) {
    uint32_t ents_per_block = fs->block_size / sizeof(ext2_bg_t);
    uint32_t block_num = blockgroup_num / ents_per_block;
    ext2_bg_t *bg_ent;

    block_num += (fs->block_size == 1024)?3:2;

    printf("ext2_get_blockgroup_descriptor(?, %d,...)\n", blockgroup_num);
    printf("  will find the data in block %d (%d entries per block)\n", block_num, ents_per_block);

    // Grab the block with the entry
    if (ext2_read_fs_block(fs, block_num) != 0) {
        printf("Failed to read ext2 block %d\n", block_num);
        return NULL;
    }

    bg_ent = (ext2_bg_t *) &fs->block_buffer[blockgroup_num * sizeof(ext2_bg_t)];

    bg->bitmap_block_lba = __builtin_bswap32(bg_ent->bitmap_block_lba);
    bg->inode_table_lba = __builtin_bswap32(bg_ent->inode_table_lba);
    bg->num_free_blocks = __builtin_bswap16(bg_ent->num_free_blocks);
    bg->num_free_inodes = __builtin_bswap16(bg_ent->num_free_inodes);
    bg->num_directories = __builtin_bswap16(bg_ent->num_directories);
    
    return bg;
}

ext2_fs_t *ext2_mount(uint8_t part_num) {
    ext2_sb_t *sb;
    int res;
    uint32_t bg1;
    uint32_t bg2;
    ext2_fs_t *fs = NULL;
    ext2_bg_t *bgt = NULL;
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

    // Calculate the number of block groups two different ways and check both
    // answers are the same.

    bg1 = sb->total_blocks / sb->blocks_per_block_group;
    if ((bg1 * sb->blocks_per_block_group) < sb->total_blocks) {
        bg1++;
    }

    bg2 = sb->total_inodes / sb->inodes_per_block_group;
    if ((bg2 * sb->inodes_per_block_group) < sb->total_inodes) {
        bg2++;
    }

    if (bg1 != bg2) {
        printf("Number of Block groups calculations inconsistency: %d != %d.\n", bg1, bg2);
        free(sb);
        return NULL;
    }

    block_size = 1024<<sb->block_size;

    fs = malloc(sizeof(ext2_fs_t));
    bgt = malloc(block_size * sb->blocks_per_block_group);
    buffer = malloc(block_size);

    if ((fs == NULL) ||( bgt == NULL) || (buffer == NULL)) {
        printf("Out of memory.\n");
        free(fs);
        free(bgt);
        free(buffer);
        free(sb);
        return NULL;
    }

    printf("ext2_mount: all ok.\n");
    // All good !
    fs->part_num = part_num;
    fs->sb = sb;
    fs->bgt = bgt;
    fs->num_blockgroups = bg1;
    fs->block_size = block_size;
    fs->block_buffer = buffer;
    fs->block_num_in_buffer = 0;
    fs->block_in_buffer_valid = 0;
    fs->sectors_per_block = fs->block_size / CF_SECTOR_SIZE;

    // Cache the Block Group Descriptor table
    
    for (uint32_t i=0; i<bg1; i++) {
        ext2_bg_t *bg = &fs->bgt[i];

        // printf("\nbgt %d @0x%08x\n", i, &fs->bgt[i]);
        if (ext2_get_blockgroup_descriptor(fs, i, bg) == NULL) {
            printf("Blargle!\n");
            free(fs->sb);
            free(fs->bgt);
            free(fs->block_buffer);
            free(fs);
            return NULL;
        }

        // printf("       Bitmap LBA: %d\n", bg->bitmap_block_lba);
        // printf("  Inode table LBA: %d\n", bg->inode_table_lba);
        // printf("    # free blocks: %d\n", bg->num_free_blocks);
        // printf("    # free inodes: %d\n", bg->num_free_inodes);
        // printf("    # directories: %d\n", bg->num_directories);
    }

    return fs;
}

ext2_fs_t *ext2_umount(ext2_fs_t *fs) {
    printf("unmount ext2 filesystem on partition %d.\n", fs->part_num);

    free(fs->sb);
    free(fs->block_buffer);
    free(fs);

    return NULL;
}