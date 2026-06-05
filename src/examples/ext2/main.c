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
#include <stddef.h>
#include <stdlib.h>
#include <machine.h>

#include "ext2.h"
#include "disk.h"

static uint8_t sbuf[CF_SECTOR_SIZE];

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

int print_dirent(ext2_dirent_t *dp) {

    ext2_sanitize_dirent(dp, dp);

    printf("Directory entry:\n");

    printf("  Inode:      %d\n", dp->inode);
    printf("  Record len: %d\n", dp->rec_len);
    printf("  Name len:   %d\n", dp->name_len);
    printf("  File type:  0x%02x\n", dp->file_type);

    printf("  File name:  ");
    for (int i=0; i<dp->name_len; i++) {
        printf("%c", dp->name[i]);
    }
    printf("\n");

    return dp->rec_len;
}

void do_dir(ext2_fs_t *fs, ext2_inode_t *in) {
    uint8_t *bp;
    int looping;

    printf("do_dir() #dir ents per block is %d\n", fs->block_size / sizeof(ext2_dirent_t));

    for (int i=0; i<12; i++) {
        if (in->i_block[i] == 0) {
            return;
        }

        if (ext2_read_fs_block(fs, in->i_block[i]) != 0) {
            printf("Failed to readd i_block[%d]: %d\n", i, in->i_block[i]);
            return;
        }

        bp = fs->block_buffer;
        looping = 1;
        while (looping) {
            int offset = print_dirent((ext2_dirent_t *) bp);

            bp += offset;
            if (bp >= &fs->block_buffer[fs->block_size]) {
                looping = 0;
            }
        }
    }
}

int main(void) {
    int res;
    int drive = 0;
    uint8_t num_partitions;

	// printf("cf_init()\r\n");
    cf_init();

    if (!cf_present()) {
        printf("No CF drives present.\n");
        exit(1);
    }

    if (drive_ready(0)) {
        printf("CF drive %d info:\n", drive);
        cf_info(drive);

        res = cf_read(drive, 0, sbuf);
        //dump(sbuf, CF_SECTOR_SIZE, 0);

        if (res != 0) {
            printf("Failed to read sector 0!\n");
            exit(1);
        }

        read_partition_table(drive);
    }

    num_partitions = get_partition_count();
    printf("Number of valid partitions found: %d\n", num_partitions);

    for (uint8_t part=0; part<num_partitions; part++) {
        ext2_fs_t *fs = ext2_mount(part);

        if (fs == NULL) {
            printf("Failed to mount partition %d as ext2\n", part);
        }
        else {
            // Do stuff
            ext2_inode_t inode;
            int res = ext2_get_inode(fs, EXT2_ROOT_INO, &inode);

            if (res != 0) {
                printf("Failed to get root inode.\n");
                exit(1);
            }

            do_dir(fs, &inode);
            fs = ext2_umount(fs);
        }
    }

    return 0;
}

