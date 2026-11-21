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
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "bitmap.h"

bitmap_t *new_bitmap(int size_in_bits) {
    bitmap_t *map;
    int size_in_bytes;

    if (size_in_bits <= 0) {
        return NULL;
    }

    size_in_bytes = (size_in_bits + 7) >> 3;

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

    map->size = size_in_bits;
    map->map = ((uint8_t *)map) + sizeof(bitmap_t);

    // Zero the bitmap (and thus mark all bits as free)
    memset(map->map, 0, size_in_bytes);

    return map;
}

void free_bitmap(bitmap_t *map) {
    if (map != NULL) {
        map->size = 0;

        free(map);
    }
}

int bitmap_fetch(bitmap_t *map, int bit_num) {
    if ((bit_num < 0) || (bit_num >= map->size)) {
        return NO;
    }

    return (map->map[bit_num >> 3] & (1 << (bit_num & 7)))?YES:NO;
}

int bitmap_set(bitmap_t *map, int bit_num, int val) {
    uint8_t mask;

    if ((bit_num < 0) || (bit_num >= map->size)) {
        return NOT_OK;
    }

    mask = (uint8_t)(1 << (bit_num & 7));

    if (val) {
        map->map[bit_num >> 3] |= mask;
    }
    else {
        map->map[bit_num >> 3] &= (uint8_t)~mask;
    }

    return OK;
}
