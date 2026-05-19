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

#include "ext2.h"
#include "disk.h"

static uint8_t sbuf[CF_SECTOR_SIZE];

void printn(const char *pfx, const uint8_t *str, int len) {
    printf(pfx);

    for (int i=0; i<len; i++) {
        putchar(str[i]);
    }
    printf("\n");
}

void dump(uint8_t *buf, size_t count) {
    int all_zeroes = 0;
    int printed_something = 0;

    for (size_t i=0; i<count; i+=16) {
        all_zeroes = 1;
        for (int x=0; x<16; x++) {
            if (buf[i+x]) {
                all_zeroes = 0;
            }
        }

        if (!all_zeroes) {
            printf("%04x: ", i);
            for (int x=0; x<16; x++) {
                printf("%02x ", buf[i+x]);
            }
            putchar('\n');

            printed_something = 1;
        }
    }

    if (!printed_something) {
        printf("Data is all zeroes.\n");
    }

    putchar('\n');
}

void cf_info(uint8_t drive_num) {
    cf_info_t info;

    // printf("Reading CF info...\n");

    if (cf_identify(drive_num, &info) != 0) {
        printf("cf_identify(%d, ...) failed.\n", drive_num);
        return;
    }

	printf("\nSignature          : %04x\n", info.signature);
    printf("   Serial          : %s\n", info.serial_num);
	printf("  Model #          : %s\n", info.model_number);
    printf(" Firmware Revision : %s\n", info.firmware_revision);
	printf(" Num Cylinders     : %d\n", info.current_num_cylinders);
	printf(" Num Heads         : %d\n", info.current_num_heads);
	printf("Capacity in sectors: %d\n", info.current_capacity_in_sectors);
    printf("\n");
}

int cf_detect(uint8_t drive_num)
{
	uint8_t status;

    if (drive_num > 1) {
        printf("Bad drive number (%d)\n", drive_num);
        return 0;
    }

	status = *cf_reg_status;

	// If the busy bit is already set, or the two bits that are always 0, then perhaps nothing is connected
	printf("status=%02x\n", status);
	if (status & (CF_ST_BUSY | 0x06)) {
		printf("It's busy and shouldn't be!\n");
		return 0;
	}

	CF_DELAY(10);

    *cf_reg_lba3 = 0xe0 | (drive_num?0x10:0);
	*cf_reg_command = CF_CMD_IDENTIFY;

	for (int i = 0; i < 1000; i++) {
		CF_DELAY(10);

		status = *cf_reg_status;
		// If it becomes unbusy within the timeout then a drive is connected
		if (!(status & CF_ST_BUSY)) {
			if (status & CF_ST_RDY) {
				CF_DELAY(100);
				return 1;
			} else {
				return 0;
			}
		}
	}
	return 0;
}

uint8_t drive_ready(uint8_t drive_num) {
    uint8_t status;

    if (drive_num > 1) {
        return 0;
    }

    *cf_reg_lba3 = 0xe0 | (drive_num?0x10:0);
    status = *cf_reg_status;
    // printf("CF Status: 0x%02x\n", status);

    return status & CF_ST_RDY;
}

int cf_present(void) {
    /* 
     * Poll the status register: if no card is present, this will
     * consistently return 0. If a card is present, DRDY and DSC
     * should be set.
     */ 
    register uint8_t cf_good = CF_ST_RDY | CF_ST_DSC;

    for (int i=0; i<1000; i++) {

        if ((*cf_reg_status & cf_good) == cf_good) {
            return 1;
        }
    }

    return 0;
}

void sanitize_superblock(ext2_sb_t *sb) {
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
}

int is_ext2(uint8_t part_num) {
    ext2_sb_t *sb = (ext2_sb_t *)sbuf;

    if (partition_read(part_num, 2, sbuf) != 0) {
        return 0;
    }

    //dump(sbuf, CF_SECTOR_SIZE);

    sanitize_superblock(sb);

    if (sb->signature != 0xef53) {
        printf("ext2 signature not found in superblock.\n");
        return 0;
    }

    printf("Root superblock:\n");
    printf("       Signature: %04x\n", sb->signature);
    printf("         Version: %d.%d\n", sb->major_version, sb->minor_version);

    if (sb->major_version >= 1) {
        // Extended superblock will be present
        printn("     Volume Name: ", sb->volume_name, sizeof(sb->volume_name));
        printn(" Last Mounted As: ", sb->last_mounted_at, sizeof(sb->last_mounted_at));
        // printn("        Block Id: ", sb->blkid, sizeof(sb->blkid));
        printn("      Journal Id: ", sb->journal_id, sizeof(sb->journal_id));
    }

    return 1;
}

int main(void) {
    //e2_superblock_t *sbp = (e2_superblock_t *)sbuff;
    int res;
    int drive = 0;
    uint8_t num_partitions;

	// printf("cf_init()\r\n");
    cf_init();

    if (!cf_present()) {
        printf("No CF drives present.\n");
        return 0;
    }

    if (drive_ready(0)) {
        printf("CF drive %d info:\n", drive);
        cf_info(drive);

        res = cf_read(drive, 0, sbuf);
        //dump(sbuf, CF_SECTOR_SIZE);

        if (res != 0) {
            printf("Failed to read sector 0!\n");
            return 1;
        }

        read_partition_table(drive);
    }

    num_partitions = get_partition_count();
    printf("Number of valid partitions found: %d\n", num_partitions);

    for (uint8_t part=0; part<num_partitions; part++) {
        if (is_ext2(part)) {
            printf("ext2 filesystem found on partition %d\n", part);
        }
    }

    return 0;
}

