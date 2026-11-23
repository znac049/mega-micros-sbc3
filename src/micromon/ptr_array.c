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
#include <errno.h>

#include "ptr_array.h"


static int grow(ptr_array_t *arr, uint16_t new_size) {
    void *new_data;
    void *old_data;

    if (new_size > arr->max_slots) {
        new_size = arr->max_slots;
    }

    if (new_size < arr->num_current_slots) {
        return NOT_OK;
    }

    new_data = malloc(new_size * sizeof(uintptr_t));
    if (new_data == NULL) {
        // out of memory
        return NOT_OK;
    }

    // Copy the old array over to the new
    old_data = arr->data;
    memcpy(new_data, old_data, arr->num_current_slots * sizeof(uintptr_t));

    arr->data = new_data;
    arr->num_current_slots = new_size;

    // Don't forget to free the old data area
    free(old_data);

    return OK;
}

static int is_free_array_slot(ptr_array_t *arr, uint16_t slot) {
    if (slot >= arr->max_slots) {
        errno = ERANGE;
        return NO;
    }

    return (bitmap_fetch(arr->bm, slot) == YES)?NO:YES;
}

static int find_free_array_slot(ptr_array_t *arr) {
    int new_size;
    int free_slot = arr->num_current_slots;

    for (int i=0; i<arr->num_current_slots; i++) {
        if (bitmap_fetch(arr->bm, i) == NO) {
            return i;
        }
    }

    // No free slots. Can we grow the array?
    if (arr->num_current_slots >= arr->max_slots) {
        // No room to grow
        return NOT_OK;
    }

    // Decide how much to grow it by
    new_size = arr->num_current_slots * 2;
    if (new_size > arr->max_slots) {
        new_size = arr->max_slots;
    }

    if (grow(arr, new_size) == NOT_OK) {
        return NOT_OK;
    }

    return free_slot;
}

ptr_array_t *new_ptr_array(uint16_t min, uint16_t max) {
    ptr_array_t *arr = malloc(sizeof(ptr_array_t));
    bitmap_t *bm = new_bitmap(max);
    uintptr_t *data;
    uint16_t size = (min < 4) ? 4 : min;

    if (size > max) {
        size = max;
    }

    data = malloc(size * sizeof(uintptr_t));

    if ((arr == NULL) || (bm == NULL) || (data == NULL)) {
        // It's safe to pass NULL to free().
        free(arr);
        free(bm);
        free(data);
        return NULL;
    }

    arr->data = data;
    arr->bm = bm;
    arr->max_slots = max;
    arr->num_current_slots = size;

    memset(arr->data, 0, size * sizeof(uintptr_t));

    return arr;
}

void free_ptr_array(ptr_array_t *arr) {
    if (arr != NULL) {
        free_bitmap(arr->bm);
        free(arr->data);
        free(arr);
    }
}

uintptr_t get_array_item(ptr_array_t *arr, uint16_t slot) {
    if (arr == NULL) {
        return NOT_OK;
    }
    
    if (slot >= arr->num_current_slots) {
        errno = ERANGE;
        return (uintptr_t)NULL;
    }

    if (bitmap_fetch(arr->bm, slot) == NO) {
        errno = ENOENT;
        return (uintptr_t)NULL;
    }

    errno = 0;
    return arr->data[slot];
}

int add_array_item(ptr_array_t *arr, uintptr_t val) {
    int slot;

    if (arr == NULL) {
        return NOT_OK;
    }
    
    slot = find_free_array_slot(arr);
    if (slot >= 0) {
        arr->data[slot] = val;
        bitmap_set(arr->bm, slot, YES);
        return slot;
    }

    return NOT_OK;
}

int free_array_item(ptr_array_t *arr, uint16_t slot) {
    if (arr == NULL) {
        return NOT_OK;
    }
    
    if (slot < arr->num_current_slots) {
        bitmap_set(arr->bm, slot, NO);
        arr->data[slot] = (uintptr_t)NULL;

        return OK;
    }

    return NOT_OK;
}
