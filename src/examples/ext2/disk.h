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

#define MAX_PARTITIONS 4

struct disk_partition {
    uint32_t    start_sector;
    uint32_t    num_sectors;
    uint8_t     flags;
    uint8_t     id;
};

typedef struct disk_partition disk_partition_t;

struct disk_info {
    uint8_t             num_partitions;
    disk_partition_t    partitions[MAX_PARTITIONS];
};

typedef struct disk_info disk_info_t;

uint8_t get_partition_count(void);
int partition_read(uint8_t part_num, uint32_t sector, uint8_t *buffer);
int read_partition_table(uint8_t drive_num);
