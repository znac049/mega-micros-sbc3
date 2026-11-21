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

#include <ctype.h>
#include <stddef.h>
#include <malloc.h>
#include <string.h>

#include "bitmap.h"

bitmap_t *new_bitmap(int size_in_bits) {
    bitmap_t *map;
    int size_in_bytes = ((size_in_bits-1) >> 3) + 1;
    uint8_t *bm;

    printf("new_bitmap(); %d bits needs %d bytes\n", size_in_bits, size_in_bytes);

    /*
     * by allocating enough memory for the struct and the bitmap
     * in one go, we save the memory overhead of a second
     * malloc.
     */
    map = malloc(sizeof(bitmap_t) + size_in_bytes);
    if (map == NULL) {
        // Out of memory
        return NULL;
    }

    // Zero the memory (and thus mark all bits as free)
    memset(map, 0, size_in_bytes);
    bm = ((uint8_t *)map) + sizeof(bitmap_t);

    map->size = size_in_bits;
    map->map = bm;

    return map;
}

bitmap_t *free_bitmap(bitmap_t *map) {
    map->size = 0;
    return free(map);
}

int bitmap_get(bitmap_t *map, int bit_num) {
    int byte_num = ((bit_num-1) >> 3) + 1;
    int bit_mask = 1 << (bit_num & 0x03);

    if ((bit_num < 0) || (bit_num >= map->size)) {
        return NO;
    }

    printf("bitmap_get(): bit number %d lives in byte %d, mask 0x%02x\n", 
        bit_num, byte_num, bit_mask);

    return (map->map[byte_num]&bit_mask)?YES:NO;
}

int bitmap_set(bitmap_t *map, int bit_num, int val) {
    int byte_num = ((bit_num-1) >> 3) + 1;
    int bit_mask = 1 << (bit_num & 0x03);
    uint8_t ent;

    if ((bit_num < 0) || (bit_num >= map->size)) {
        return NOT_OK;
    }

    printf("bitmap_get(): bit number %d lives in byte %d, mask 0x%02x\n", 
        bit_num, byte_num, bit_mask);

    ent = map->map[byte_num];
    map->map[byte_num] = (val)? ent | bit_mask : ent & (~bit_mask); 

    return OK;
}