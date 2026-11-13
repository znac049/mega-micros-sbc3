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

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <machine.h>

#include "micromon.h"
#include "cf_block.h"

extern disk_info_t dk_info;

static int cf_dev_init(block_device_t *dev) {
    // this was done earlier

    dev->active = YES;

    return OK;
}

static int cf_dev_finish(void) {
    return -1;
}

static int cf_dev_read_block(block_device_t *dev, uint32_t block_num, uint8_t *buff) {
    uint32_t sector = block_num * SECTORS_PER_BLOCK;

    // printk("CF_dev_read_block() block # = %d\n", block_num);
    // printk("block %d -> sector %d\n", block_num, sector);

    for (int i=0; i<SECTORS_PER_BLOCK; i++) {
        int res = cf_read(dev->private.data.cf.drive_num, 
                            sector + dev->private.data.cf.start_sector, buff);
        if (res != OK) {
            printk("Blargle - %d!\n", res);
            return NOT_OK;
        }

        buff = &buff[CF_SECTOR_SIZE];
        sector++;
    }

    return OK;
}

static int cf_dev_write_block(block_device_t *dev, uint32_t block_num, uint8_t *buff) {
    (void)dev;
    (void)block_num;
    (void)buff[0];
    
    return NOT_OK;
}

static block_device_t *new_dev(void) {
    block_device_t *dev = find_free_device_slot();

    if (dev == NULL) {
        return NULL;
    }

    dev->init = cf_dev_init;
    dev->finish = cf_dev_finish;

    dev->read_block = cf_dev_read_block;
    dev->write_block = cf_dev_write_block;

    strcpy(dev->name, "cf");

    dev->private.data.cf.start_sector = 0;
    dev->private.data.cf.num_sectors = 0;

    dev->active = YES;

    return dev;
}

static int probe_cf_card(int drive_num) {
    if (cf_drive_ready(drive_num) == YES) {
        int table_type;

        printk("CF card #%d - present\n", drive_num);

        // Is it a raw disk or does it have partitions
        if ((table_type = has_partition_table(drive_num)) >= YES) {
            printk("Partition table found on CF drive %d\n", drive_num);

            if (read_partition_table(drive_num) != OK) {
                printk("Failed to read partition table :-(\n");
                return NOT_OK;
            };

            printk("number of partitions found: %d\n", dk_info.num_partitions);
            for (int i=0; i<dk_info.num_partitions; i++) {
                block_device_t *dev = new_dev();

                if (dev == NULL) {
                    printk("Couldn't grab a block_device_t entry\n");
                    return NOT_OK;
                }

                dev->private.data.cf.start_sector = dk_info.partitions[i].start_sector;
                dev->private.data.cf.num_sectors = dk_info.partitions[i].num_sectors;

                snprintf(dev->name, sizeof(dev->name), "cf%d%c", drive_num, 'a'+i);
            }
        }
        else {
            block_device_t *dev = new_dev();

            if (dev == NULL) {
                return NOT_OK;
            }

            snprintf(dev->name, sizeof(dev->name), "cf%d", drive_num);
        }
    }

    return OK;
}

int create_cf_devs(void) {
    if (cf_init() == NOT_OK) {
        printk("No CF cards present (controller not responding)\n");
        return OK;
    }

    probe_cf_card(0);
    probe_cf_card(1);

    return OK;
}
