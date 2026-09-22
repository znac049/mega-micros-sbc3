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

#define SH1107_ADDR   0x3C

#define SH1107_WIDTH    128
#define SH1107_HEIGHT   128
#define SH1107_PAGES    (SH1107_HEIGHT / 8)

/* 
 * Some 128x128 SH1107 modules need a display-offset and/or a column-offset
 * to line up correctly. The following values work for my generic baord
 * but If your image is shifted or wrapped, try adjusting these two first.
 */
#define SH1107_DISPLAY_OFFSET   0x60
#define SH1107_COLUMN_OFFSET    0x60

struct font {
    const uint8_t width;
    const uint8_t height;
    const uint16_t *font_chars;
    const uint8_t *char_widths;
};

typedef struct font font_t;


int sh1107_init(void);
void sh1107_clear(void);
void sh1107_set_pixel(int x, int y, int color);
int sh1107_pch(int x, int y, char c, font_t *font);
void sh1107_pstr(int x, int y, char *str, font_t *font);
void sh1107_display(void);
