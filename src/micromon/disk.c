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
#include <string.h>

#include "micromon.h"

disk_info_t dk_info;

disk_partition_t *get_partition(int part_num) {
    if (part_num >= dk_info.num_partitions) {
        return NULL;
    }

    return &dk_info.partitions[part_num];
}

static uint32_t le32(const uint8_t *p) {
    return (uint32_t)p[0]         |
           ((uint32_t)p[1] << 8)  |
           ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

static uint64_t le64(const uint8_t *p) {
    return le32(p) | ((uint64_t)le32(p + 4) << 32);
}

// GPT Types, in on-disk order
static const uint8_t GPT_TYPE_LINUX[16] = {
    0xAF, 0x3D, 0xC6, 0x0F, 0x83, 0x84, 0x72, 0x47,
    0x8E, 0x79, 0x3D, 0x69, 0xD8, 0x47, 0x7D, 0xE4
};

static const uint8_t GPT_TYPE_EFI[16] = {
    0x28, 0x73, 0x2A, 0xC1, 0x1F, 0xF8, 0xD2, 0x11,
    0xBA, 0x4B, 0x00, 0xA0, 0xC9, 0x3E, 0xC9, 0x3B
};


static uint8_t gpt_type_to_id(const uint8_t *guid) {
    if (memcmp(guid, GPT_TYPE_LINUX, 16) == 0) {
        return 0x83;
    }

    if (memcmp(guid, GPT_TYPE_EFI, 16) == 0) {
        return 0xef;
    }

    return 0xff;            // something else
}

static int has_mbr_sig(uint8_t drive_num) {
    uint8_t buff[CF_SECTOR_SIZE];

    if (cf_read(drive_num, 0, buff) != OK) {
        printk("Failed to read sector 0 on drive %d.\n", drive_num);
        errno = ENOENT;
        return NOT_OK;
    }

    if ((buff[510] == 0x55) && (buff[511] == 0xaa)) {
        return YES;
    }

    return NO;
}

static int has_gpt_sig(uint8_t drive_num) {
    uint8_t buff[CF_SECTOR_SIZE];

    if (cf_read(drive_num, 1, buff) != OK) {
        printk("Failed to read sector 1 on drive %d.\n", drive_num);
        errno = ENOENT;
        return NOT_OK;
    }

    buff[8] = EOS;
    if (strcmp((char *)buff, "EFI PART") == 0) {
        return YES;
    }

    printk("GPT Signature not found. Got '%s' instead\n", buff);

    return NO;
}

int has_partition_table(uint8_t drive_num) {
    int gpt = has_gpt_sig(drive_num);
    int mbr = has_mbr_sig(drive_num);

    if ((gpt == YES) && (mbr == YES)) {
        printk("MBR and GPT found\n");
        return YES;
    }
    else if (mbr == YES) {
        printk("MBR found\n");
        return YES;
    }
    
    printk("No partition table found\n");

    return NO;
}

static int add_partition(uint8_t drive_num, uint32_t start, uint32_t count, uint8_t flags, uint8_t id) {
    int ind = dk_info.num_partitions;

    if (ind >= MAX_PARTITIONS) {
        printk("Partition table full - ignoring the rest\n");
        errno = ENOMEM;
        return NOT_OK;
    }

    dk_info.partitions[ind].start_sector = start;
    dk_info.partitions[ind].num_sectors = count;
    dk_info.partitions[ind].flags = flags;
    dk_info.partitions[ind].id = id;
    snprintf(dk_info.partitions[ind].name, sizeof(dk_info.partitions[ind].name), "CF%d%c", drive_num, 'a'+ind);

    dk_info.num_partitions++;

    return OK;
}

static int read_gpt(uint8_t drive_num, uint8_t *buff) {
    static const uint8_t zero[16] = { 0 };
    uint64_t array_lba;
    uint32_t num_entries;
    uint32_t entry_size;
    uint32_t per_sector;

    // buff holds sector 0 (the protective MBR): first partition should be type $EE
    if (buff[446 + 4] != 0xee) {
        printk("First MBR partition type should be 0xEE. It is, instead, 0x%02x\n", buff[446 + 4]);
        errno = ENOENT;
        return NOT_OK;
    }

    // The GPT header is in sector 1
    if (cf_read(drive_num, 1, buff) != OK) {
        printk("Failed to read sector 1 on drive %d.\n", drive_num);
        errno = EIO;
        return NOT_OK;
    }

    array_lba = le64(&buff[72]);
    num_entries = le32(&buff[80]);
    entry_size = le32(&buff[84]);

    if ((entry_size < 128) || ((CF_SECTOR_SIZE % entry_size) != 0) || ((array_lba >> 32) != 0)) {
        printk("Unsupported GPT header (entry size %d)\n", entry_size);
        errno = EINVAL;
        return NOT_OK;
    }

    per_sector = CF_SECTOR_SIZE / entry_size;

    for (uint32_t i=0; i<num_entries; i++) {
        uint8_t *e = &buff[(i % per_sector) * entry_size];

        if ((i % per_sector) == 0) {
            if (cf_read(drive_num, (uint32_t)array_lba + (i / per_sector), buff) != OK) {
                errno = EIO;
                return NOT_OK;
            }
        }

        if (memcmp(e, zero, 16) == 0) {
            continue;                               // unused slot
        }

        uint64_t first = le64(&e[32]);
        uint64_t last = le64(&e[40]);

        if (((first >> 32) != 0) || ((last >> 32) != 0) || (last < first)) {
            printk("GPT entry %d is beyond 2TB or invalid - skipped\n", i + 1);
            continue;
        }

        if (add_partition(drive_num, (uint32_t)first, (uint32_t)(last - first + 1), 0, gpt_type_to_id(e)) != OK) {
            break;
        }
    }

    return OK;
}

static int read_mbr(uint8_t drive_num, uint8_t *buff) {
    for (int part_num=0; part_num<4; part_num++) {
        uint8_t *part = &buff[446 + (part_num<<4)];
        uint32_t start_lba = __builtin_bswap32(*((uint32_t *)&part[8]));
        uint32_t num_sectors = __builtin_bswap32((*(uint32_t *)&part[12]));

        if (num_sectors) {
            if (add_partition(drive_num, start_lba, num_sectors, part[0], part[4]) != OK) {
                break;
            }
        }
    }

    return OK;
}

int read_partition_table(uint8_t drive_num) {
    int gpt = has_gpt_sig(drive_num);
    int mbr = has_mbr_sig(drive_num);
    uint8_t buff[CF_SECTOR_SIZE];

    dk_info.num_partitions = 0;

    if (mbr != YES) {
        errno = ENOENT;
        return NOT_OK;
    }

    // Both types of partition table need sector 0 first
    if (cf_read(drive_num, 0, buff) != OK) {
        printk("Failed to read sector 0 on drive %d.\n", drive_num);
        errno = EIO;
        return NOT_OK;
    }

    if (gpt == YES) {
        return read_gpt(drive_num, buff);
    }
    else if (mbr == YES) {
        return read_mbr(drive_num, buff);
    }

    errno = ENOENT;
    return NOT_OK;
}
