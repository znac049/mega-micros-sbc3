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

#include <stddef.h>
#include <ctype.h>
#include <filesystems.h>
#include <machine.h>

#if defined(BAREMETAL)

block_device_t block_devices[MAX_BLOCK_DEVICES];

static block_device_t *find_free_device_slot(void) {
    for (int i=0; i<MAX_BLOCK_DEVICES; i++) {
        if (block_devices[i].active == NO) {
            return &block_devices[i];
        }
    }

    return NULL;
}

int bd_init(void) {
    for (int i=0; i<MAX_BLOCK_DEVICES; i++) {
        block_devices[i].active = NO;
        block_devices[i].num_sub_devices = 0;
        block_devices[i].name[0] = EOS;
    }

    create_rom_dev(find_free_device_slot());
    create_cf_dev(find_free_device_slot());

    // Initialise each block device in turn
    printf("Attempting to initialise all registered block devices\n");
    for (int i=0; i<MAX_BLOCK_DEVICES; i++) {
        block_device_t *bd = &block_devices[i];

        if (bd->active == YES) {
            if (bd->init(bd) == NOT_OK) {
                kprintf("%s init failed!\n", bd->name);

                bd->active = NO;
            }
            else {
                kprintf("\nInit block device %d '%s':", i, bd->name);
                for (uint8_t i=0; i<bd->num_sub_devices; i++) {
                    kprintf("%s%d ", bd->name, i);
                }
                kprintf("\n");
            }
        }
    }

    return 0;
}

int bd_read(block_device_t *dev, uint32_t block_num, uint8_t *buff, uint8_t subdev) {
    kprintf("bd_read()...\n");

    if ((dev != NULL) && (dev->active == YES) && (subdev < dev->num_sub_devices)) {
        kprintf("...dev='%s%d', dev->name, subdev, blk # = %d\n", dev->name, subdev, block_num);

        return dev->read_block(block_num, buff, subdev);
    }

    return NOT_OK;
}

#endif