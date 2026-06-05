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

#include <stdio.h>
#include <vectors.h>

unsigned int get_isr_handler(int vector_number) {
    unsigned int *vector_base;

    if (vector_number > 255) {
        return 0xffffffff;
    }

    vector_base = _get_vectors_base(); 

    return vector_base[vector_number];
}

unsigned int set_isr_handler(int vector_number, unsigned int isr) {
    unsigned int *vector_base;
    unsigned int old_isr;

    if (vector_number > 255) {
        return 0xffffffff;
    }

    vector_base = _get_vectors_base(); 

    old_isr = vector_base[vector_number];
    vector_base[vector_number] = isr;

    return old_isr;
}