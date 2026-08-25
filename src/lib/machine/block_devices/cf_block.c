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

#if defined(BAREMETAL)

#define SECTORS_PER_BLOCK (BLOCK_DEVICE_BLOCK_SIZE / CF_SECTOR_SIZE)

struct cf_disks_data {
    uint32_t    num_disks;
};

typedef struct cf_disks_data cf_disks_data_t;

static cf_disks_data_t cf_disks;


static int cf_dev_init(block_device_t *dev) {
    uint8_t card;
    uint8_t num_cards = 0;

    cf_init();

    for (card=0; card<2; card++) {
        if (cf_drive_ready(card) == YES) {
            kprintf("CF card #%d - present\n", card);
            num_cards++;
        }
    }

    cf_disks.num_disks = num_cards;
    dev->num_sub_devices = num_cards;

    if (num_cards == 0) {
        kprintf("No CF cards present\n");
        dev->active = NO;

        return OK;
    }

    return OK;
}

static int cf_dev_finish(void) {
    return -1;
}

static int cf_dev_read_block(uint32_t block_num, uint8_t *buff, uint8_t subdev) {
    uint32_t sector = block_num * SECTORS_PER_BLOCK;

    kprintf("CF_dev_read_block() block # = %d, subdev=%d\n", block_num, subdev);
    kprintf("block %d -> sector %d\n", block_num, sector);

    for (int i=0; i<SECTORS_PER_BLOCK; i++) {
        if (cf_read(subdev, sector, buff) == NOT_OK) {
            kprintf("Blargle!\n");
            return NOT_OK;
        }

        buff = &buff[CF_SECTOR_SIZE];
        sector++;
    }

    return OK;
}

static int cf_dev_write_block(uint32_t block_num, uint8_t *buff, uint8_t subdev) {
    return NOT_OK;
}

int create_cf_dev(block_device_t *dev) {
    if (dev == NULL) {
        return NOT_OK;
    }

    dev->init = cf_dev_init;
    dev->finish = cf_dev_finish;

    dev->read_block = cf_dev_read_block;
    dev->write_block = cf_dev_write_block;

    strcpy(dev->name, "cf");

    dev->driver_data = &cf_disks;
    dev->num_sub_devices = 2;       // Max

    dev->active = YES;

    return OK;
}

#endif