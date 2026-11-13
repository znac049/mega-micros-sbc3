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

/* Duart Register Base Addresses */
#define hex_display_base ((volatile uint8_t*) 0xab0000)

#define hex_byte_0          ((volatile uint8_t *) hex_display_base)
#define hex_byte_1          ((volatile uint8_t *) hex_display_base + 1)
#define hex_byte_2          ((volatile uint8_t *) hex_display_base + 2)
#define hex_byte_3          ((volatile uint8_t *) hex_display_base + 3)

#define hex_word_0          ((volatile uint16_t *) hex_display_base)
#define hex_word_1          ((volatile uint16_t *) hex_display_base + 2)

#define hex_long_0          ((volatile uint32_t *) hex_display_base)

#define HEXB0(val) (*hex_byte_0 = (val))
#define HEXB1(val) (*hex_byte_1 = (val))
#define HEXB2(val) (*hex_byte_2 = (val))
#define HEXB3(val) (*hex_byte_3 = (val))

#define HEXW0(val) (*hex_word_0 = (uint16_t)(val))
#define HEXW1(val) (*hex_word_1 = (uint16_t(val))

#define HEXL0(val) (*hex_long_0 = (uint32_t)(val))
