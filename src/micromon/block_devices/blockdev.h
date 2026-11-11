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

#pragma once

#define BLOCK_DEVICE_BLOCK_SIZE 2048
#define BLOCK_NUM_SHIFT 11
#define MAX_BLOCK_DEVICES 4


// typedef struct device_block device_block_t;
typedef struct block_device block_device_t;


struct block_device {
    bool_t active;
    char name[16];
    uint8_t num_sub_devices;

    int (*init)(block_device_t *dev);
    int (*finish)(void);

    int (*read_block)(uint32_t block_num, uint8_t *buff, uint8_t subdev);
    int (*write_block)(uint32_t block_num, uint8_t *buff, uint8_t subdev);

    void *driver_data;
};



extern block_device_t block_devices[MAX_BLOCK_DEVICES];