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

typedef struct {
    uint32_t d0;        // 0
    uint32_t d1;        // 4
    uint32_t d2;        // 8
    uint32_t d3;        // 12
    uint32_t d4;        // 16
    uint32_t d5;        // 20
    uint32_t d6;        // 24
    uint32_t d7;        // 28
    uint32_t a0;        // 32
    uint32_t a1;        // 36
    uint32_t a2;        // 40
    uint32_t a3;        // 44
    uint32_t a4;        // 48
    uint32_t a5;        // 52
    uint32_t a6;        // 56
    uint32_t sp;        // 60
    uint32_t ra;        // 64
    uint16_t sr;        // 68
} jmp_buf[1];
 
int  setjmp(jmp_buf env);
void longjmp(jmp_buf env, int val) __attribute__((noreturn));
