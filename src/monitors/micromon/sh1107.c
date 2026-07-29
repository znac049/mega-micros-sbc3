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


#include <stddef.h>
#include <ctype.h>
#include <machine.h>

#include "micromon.h"
#include "fonts.h"

static uint8_t framebuffer[SH1107_PAGES][SH1107_WIDTH];

static font_t norm_font = {15, 16, med_font, med_widths};


static void reset_delay(void) {
    volatile int i;

    for (i = 0; i < 1000; i++) {
        ;
    }
}

static int i2c_failed(void) {
    i2c_stop();

    return -1;
}

/* Returns 0 on success, -1 if any byte was NACKed. */
static int sh1107_command(uint8_t cmd) {
    i2c_start();

    if (i2c_write_byte((uint8_t)(SH1107_ADDR << 1))) {
        return i2c_failed();
    }

    if (i2c_write_byte(0x00)) {
        return i2c_failed();
    }

    if (i2c_write_byte(cmd)) {
        return i2c_failed();
    }

    i2c_stop();

    return 0;
}

static int sh1107_data_burst(const uint8_t *buf, int len) {
    int i;

    i2c_start();

    if (i2c_write_byte((uint8_t)(SH1107_ADDR << 1))) {
        return i2c_failed();
    }

    if (i2c_write_byte(0x40u)) {
        return i2c_failed();
    }

    for (i = 0; i < len; i++) {
        if (i2c_write_byte(buf[i])) {
            return i2c_failed();
        }
    }

    i2c_stop();

    return 0;
}

int sh1107_init(void) {
    int err = 0;

    i2c_init();

    err |= sh1107_command(0xAE);                    /* display off */
    err |= sh1107_command(0xDC);                    /* set display start line */
    err |= sh1107_command(0x00);
    err |= sh1107_command(0x81);                    /* set contrast */
    err |= sh1107_command(0x2F);
    err |= sh1107_command(0x20);                    /* memory addressing: page mode */
    err |= sh1107_command(0xA0);                    /* segment remap: normal */
    err |= sh1107_command(0xC0);                    /* COM scan direction: normal */
    err |= sh1107_command(0xA8);                    /* set multiplex ratio */
    err |= sh1107_command((uint8_t)(SH1107_HEIGHT - 1));
    err |= sh1107_command(0xD3);                    /* set display offset */
    err |= sh1107_command(SH1107_DISPLAY_OFFSET);
    err |= sh1107_command(0xD5);                    /* set display clock divide */
    err |= sh1107_command(0x51);
    err |= sh1107_command(0xD9);                    /* set pre-charge period */
    err |= sh1107_command(0x22);
    err |= sh1107_command(0xDB);                    /* set VCOM deselect level */
    err |= sh1107_command(0x35);
    err |= sh1107_command(0xAD);                    /* set DC-DC (charge pump) */
    err |= sh1107_command(0x8A);
    err |= sh1107_command(0xA6);                    /* normal (not inverted) display */
    err |= sh1107_command(0xA4);                    /* resume to RAM content display */

    err |= sh1107_command(0xAF);                    /* display on */

    return err ? -1 : 0;                            /* -1 = no ACK somewhere -- check wiring/address */
}

static void sh1107_set_page(uint8_t page) {
    sh1107_command((uint8_t)(0xB0 | (page & 0x0F)));
}

static void sh1107_set_column(uint8_t col) {
    uint8_t c = (uint8_t)(col + SH1107_COLUMN_OFFSET);
    sh1107_command((uint8_t)(0x00 | (c & 0x0F)));         /* column low nibble */
    sh1107_command((uint8_t)(0x10 | ((c >> 4) & 0x0F)));  /* column high nibble */
}

void sh1107_clear(void) {
    int p, c;

    for (p = 0; p < SH1107_PAGES; p++)
        for (c = 0; c < SH1107_WIDTH; c++)
            framebuffer[p][c] = 0;
}

/* color: 0 = pixel off, non-zero = pixel on. (0,0) is top-left. */
void sh1107_set_pixel(int x, int y, int color) {
    int page;
    int bit;

    if ((x < 0) || (x >= SH1107_WIDTH) || (y < 0) || (y >= SH1107_HEIGHT))
        return;

    page = y / 8;
    bit  = y % 8;

    if (color) {
        framebuffer[page][x] |= (uint8_t)(1 << bit);
    }
    else {
        framebuffer[page][x] &= (uint8_t)~(1 << bit);
    }
}

int sh1107_pch(int x, int y, char c, font_t *font) {
    int char_offset;
    uint8_t wid;

    if ((x < 0) || (x >= SH1107_WIDTH) || (y < 0) || (y >= SH1107_HEIGHT))
        return 0;

    if ((c < ' ') || (c > '~')) {
        c = ' ';
    }

    char_offset = (c - ' ') * font->width;
    wid = font->char_widths[c - ' '];

    for (int i=0; i<font->height; i++) {
        uint16_t row = font->font_chars[char_offset+i];
        uint16_t mask_bit = 1<<15;
    
        // prrrr(row, wid);

        for (int j=0; j<wid; j++) {
            sh1107_set_pixel(x+j, y+i, row & mask_bit);
            mask_bit = mask_bit >> 1;
        }
    }

    return wid;
}

void sh1107_pstr(int x, int y, char *str, font_t *font) {
    if (font == NULL) {
        font = &norm_font;
    }

    while (*str) {
        uint8_t ch = *str++;

        if (ch == '\n') {
            x = 0;
            y += font->height;
        }
        else {
            x += sh1107_pch(x, y, ch, font);
        }

        if (x >= SH1107_WIDTH) {
            x = 0;
            y += font->height;

            if (y >= SH1107_HEIGHT) {
                y = 0;
            }
        }
    }
}
/* Pushes the whole framebuffer to the display, one page at a time. */
void sh1107_display(void) {
    int p;

    for (p = 0; p < SH1107_PAGES; p++) {
        sh1107_set_page((uint8_t)p);
        sh1107_set_column(0);
        sh1107_data_burst(framebuffer[p], SH1107_WIDTH);
    }
}