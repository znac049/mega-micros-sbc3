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

extern uint32_t _data_load_start;
extern uint32_t _data_length;

#define ROM_BASE 0xC00000
#define ROM_END (ROM_BASE + (1024 * 1024 *4))

#if defined(BAREMETAL)

struct rom_disk_data {
    uint32_t    num_blocks;
    uint8_t    *base;
};

typedef struct rom_disk_data rom_disk_data_t;

static rom_disk_data_t rom_disk;

static uint8_t *find_start_of_disk(void) {
    uint32_t start = (uint32_t)&_data_load_start;
    uint32_t len = (uint32_t)&_data_length;
    uint8_t *base = (uint8_t *)(start+len);

    // kprintf("base=$%06X ($%06X + %06X)\n", base, start, len);

    // Hunt the pre-disk signature
    for (int i=0; i<1024; i++) {
        if (
                (base[0] == 0xde) && 
                (base[1] == 0xad) &&
                (base[2] == 0xfa) &&
                (base[3] == 0xce) &&
                (base[4] == 0xc0) &&
                (base[5] == 0x1d) &&
                (base[6] == 0xbe) &&
                (base[7] == 0xef)
            ) {
            // kprintf("Found signature at $%06X!!!\n", base);

            return base+8;
        }

        base++;
    }

    return NULL;
}

static int rom_dev_init(block_device_t *dev) {
    uint8_t *rom_disk_base = find_start_of_disk();

    if (rom_disk_base == NULL) {
        // kprintf("ROM disk signature not found\n");

        return NOT_OK;
    }

    // set the start of disk - point to the data immediately after the monitor
    rom_disk.base = rom_disk_base;

    // Figure out how big the 'disk' is, in blocks
    rom_disk.num_blocks = (ROM_END - (uint32_t)rom_disk_base) / BLOCK_DEVICE_BLOCK_SIZE; 
    dev->num_sub_devices = 1;

    kprintf("ROM disk @ $%08X, # blocks=%d, read-only\n", rom_disk.base, rom_disk.num_blocks);

    return OK;
}

static int rom_dev_finish(void) {
    return OK;
}

static int rom_dev_read_block(uint32_t block_num, uint8_t *buff, uint8_t subdev) {
    uint8_t *src = rom_disk.base;
    uint32_t offset = block_num*BLOCK_DEVICE_BLOCK_SIZE;

    // kprintf("rom_dev_read_block() block # = %d, subdev=%d\n", block_num);

    if (block_num >= rom_disk.num_blocks) {
        kprintf("block %d out of range\n", block_num);
        return NOT_OK;
    }

    src += offset;
    // kprintf("block %d starts at $%08x, offset=%d\n", block_num, src, offset);

    memcpy(buff, src, BLOCK_DEVICE_BLOCK_SIZE);

    return OK;
}

static int rom_dev_write_block(uint32_t block_num, uint8_t *buff, uint8_t subdev) {
    // Doh - can't write to the ROM

    return NOT_OK;
}

int create_rom_dev(block_device_t *dev) {
    if (dev == NULL) {
        return NOT_OK;
    }
    
    dev->init = rom_dev_init;
    dev->finish = rom_dev_finish;

    dev->read_block = rom_dev_read_block;
    dev->write_block = rom_dev_write_block;

    strcpy(dev->name, "ROM");

    dev->driver_data = &rom_disk;
    dev->num_sub_devices = 1;

    dev->active = YES;

    return OK;
}

#endif

