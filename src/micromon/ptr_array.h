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

#include <ctype.h>

#include "bitmap.h"

struct ptr_array {
    bitmap_t *bm;
    uint16_t max_slots;
    uint16_t num_current_slots;
    uintptr_t *data;
};

typedef struct ptr_array ptr_array_t;


ptr_array_t *new_ptr_array(uint16_t min, uint16_t max);
void free_ptr_array(ptr_array_t *arr);
uintptr_t get_array_item(ptr_array_t *arr, uint16_t slot);
int add_array_item(ptr_array_t *arr, uintptr_t val);
int free_array_item(ptr_array_t *arr, uint16_t slot);
int array_has_free_slot(ptr_array_t *arr);