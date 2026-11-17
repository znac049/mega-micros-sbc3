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


#define RAM_LOW_BASE        0x00000000
#define RAM_HIGH_BASE       0x01000000
#define RAM_AREA_MAX        EIGHT_MEG

/*
 * Where in each area to test: in the low area this is vector 252, which
 * nothing uses. It must not be anywhere this code's own stack frame
 * could be.
 */
#define RAM_PROBE_OFFSET    0x3f0

#define MARK_BASE           0x5a5aa5a5U
#define MARK_ALIAS          0xc33c3cc3U

// 68030 CACR data cache bits
#define CACR_ED             0x0100      // data cache enabled
#define CACR_CD             0x0800      // clear data cache

#define RAM_MAX ((1024*1024*8)-1)
#define ONE_MEG (1024*1024)
#define TWO_MEG (ONE_MEG*2)
#define THREE_MEG (ONE_MEG*3)
#define FOUR_MEG (ONE_MEG*4)
#define FIVE_MEG (ONE_MEG*5)
#define EIGHT_MEG (ONE_MEG*8)
