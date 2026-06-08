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
#include <errno.h>
#include <machine.h>

#include "disk.h"

disk_info_t dk_info;

uint8_t get_partition_count(void) {
    return dk_info.num_partitions;
}

disk_partition_t *get_partition(int part_num) {
    if (part_num >= dk_info.num_partitions) {
        return NULL;
    }

    return &dk_info.partitions[part_num];
}

int read_partition_table(uint8_t drive_num) {
    uint8_t buf[CF_SECTOR_SIZE];

    if (cf_read(drive_num, 0, buf) != 0) {
        // printf("Failed to read partition table on drive %d.\n", drive_num);
        errno = ENOENT;
        return -1;
    }

    if ((buf[510] != 0x55) || (buf[511] != 0xaa)) {
        // printf("No MSDOS partition flag (0x55aa) found.\n");
        errno = ENOENT;
        return -1;
    }

    for (int part_num=0; part_num<4; part_num++) {
        uint8_t *part = &buf[446 + (part_num<<4)];
        uint32_t start_lba = __builtin_bswap32(*((uint32_t *)&part[8]));
        uint32_t num_sectors = __builtin_bswap32((*(uint32_t *)&part[12]));

        if (num_sectors) {
            int ind = dk_info.num_partitions;

            if (ind >= MAX_PARTITIONS) {
                // printf("Partition table full!\n");
                errno = ENOMEM;
                return -1;
            }

            dk_info.partitions[ind].start_sector = start_lba;
            dk_info.partitions[ind].num_sectors = num_sectors;
            dk_info.partitions[ind].flags = part[0];
            dk_info.partitions[ind].id = part[4];
            snprintf(dk_info.partitions[ind].name, sizeof(dk_info.partitions[ind].name), "CF%d%c", drive_num, 'a'+part_num);

            dk_info.num_partitions++;
        }
    }

    return 0;
}

int partition_read(uint8_t part_num, uint32_t sector, uint8_t *buffer) {
    if (part_num >= dk_info.num_partitions) {
        // printf("Partition number out of range.\n");
        errno = EINVAL;
        return -1;
    }

    if (sector >= dk_info.partitions[part_num].num_sectors) {
        // printf("sector number out of range!\n");
        errno = EINVAL;
        return -1;
    }

    // printf("partition_read(%d, %d,...)\n", part_num, sector);

    return cf_read(0, dk_info.partitions[part_num].start_sector + sector, buffer);
}