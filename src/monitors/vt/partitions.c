#include <stdio.h>
#include <string.h>
#include <machine.h>

#include "cli.h"

partition_t partitions[4];

int get_partition_count(void) {
    int res = 0;

    for (int i=0; i<4; i++) {
        if (partitions[i].num_sectors) {
            res++;
        }
    }

    return res;
}

int has_mbr(int disk_id) {
    MBR_t *mbr = (MBR_t *)sector_buffer;

    if (cf_read(0, 0, sector_buffer) != 0) {
		printf("CF read failed\n");
		return -1;
	}

    if (mbr->signature != 0x55aa) {
        return 0;
    }

    /* 
        The signature is present, but as a further check, see if there's
        at least one partition table with a non-zero number of sectors
    */
    memcpy(partitions, mbr->partition_table, sizeof(mbr->partition_table));

    return get_partition_count();
}

int is_fat16(void) {
    if ((sector_buffer[0] != 0xeb) || (sector_buffer[2] != 0x90)) {
        return PART_NONE;
    }

    if ((sector_buffer[38] != 0x28) && (sector_buffer[38] != 0x29)) {
        return PART_NONE;;
    }

    return ((sector_buffer[510] == 0x55) && (sector_buffer[511] == 0xaa));
}

int get_partition_type(int disk_id, int partition_num) {
    if ((partition_num < 0) || (partition_num>= 4)) {
        return PART_NONE;
    }

    if (has_mbr(disk_id)) {
        if (partitions[partition_num].num_sectors > 0) {
            // It is a partition

            if (cf_read(0, partitions[partition_num].lba_start, sector_buffer)) {
                return PART_NONE;
            }

            dump_sector(partitions[partition_num].lba_start);

            // Inspect the buffer
            if (is_fat16()) {
                printf("It's FAT16\n");
                return PART_FAT16;
            }            
        }
    }

    return PART_NONE;
}