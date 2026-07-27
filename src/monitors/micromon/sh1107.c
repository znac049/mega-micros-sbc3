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

/*-----------------------------------------------------------------------
 *
 * I2C framing per the SH1107 datasheet: after the slave address, each
 * command or data byte is preceded by a "control byte":
 *   bit 7 (Co)  : 1 = another control byte follows, 0 = only data
 *                 bytes follow from here on
 *   bit 6 (D/C) : 0 = the following byte is a command,
 *                 1 = the following byte is display-RAM data

 * This driver uses one I2C transaction per command (control byte
 * 0x00 + command byte), and a single control byte 0x40 followed by a
 * burst of data bytes when pushing pixel data -- simple and reliable,
 * at the cost of a few extra I2C transactions during init.
 *
 * Default 7-bit address is 0x3C (SA0 tied low); some modules use
 * 0x3D (SA0 tied high).
 *
 * RESET: the SH1107 also has a hardware RES pin. Some breakout boards
 * expose it to a GPIO, others tie it straight to VCC (no software
 * control). This driver assumes it's wired to PB0 on the 68230's
 * Port B -- if your module ties RES to VCC instead, delete
 * sh1107_reset() and its call in sh1107_init().
 *-----------------------------------------------------------------------*/

#include <stdint.h>

/* ---- from i2c_bitbang_68230.c -- link against that file ---- */
extern void    i2c_init(void);
extern void    i2c_start(void);
extern void    i2c_stop(void);
extern int     i2c_write_byte(uint8_t byte);   /* 0 = ACK, 1 = NACK */

/*-----------------------------------------------------------------------
 * Reset pin (Port B, bit I/O submode -- see sh1107_spi.c for the same
 * pattern used with more pins). Delete this block if RES is hardwired.
 *-----------------------------------------------------------------------*/
#define PIT_BASE   0xAF0001UL

#define PGCR       (*(volatile uint8_t *)(PIT_BASE + 0x00))
#define PBDDR      (*(volatile uint8_t *)(PIT_BASE + 0x06))
#define PBDR       (*(volatile uint8_t *)(PIT_BASE + 0x12))

#define RES_BIT    0
#define RES_MASK   (1u << RES_BIT)

static void reset_delay(void)
{
    volatile int i;
    for (i = 0; i < 1000; i++)
        ;
}

/*-----------------------------------------------------------------------
 * I2C transaction helpers
 *-----------------------------------------------------------------------*/

#define SH1107_I2C_ADDR   0x3Cu

/* Returns 0 on success, -1 if any byte was NACKed. */
static int sh1107_command(uint8_t cmd)
{
    i2c_start();
    if (i2c_write_byte((uint8_t)(SH1107_I2C_ADDR << 1))) goto fail;
    if (i2c_write_byte(0x00u))                            goto fail; /* Co=0, D/C=0 */
    if (i2c_write_byte(cmd))                              goto fail;
    i2c_stop();
    return 0;

fail:
    i2c_stop();
    return -1;
}

static int sh1107_data_burst(const uint8_t *buf, int len)
{
    int i;

    i2c_start();
    if (i2c_write_byte((uint8_t)(SH1107_I2C_ADDR << 1))) goto fail;
    if (i2c_write_byte(0x40u))                            goto fail; /* Co=0, D/C=1 */
    for (i = 0; i < len; i++)
        if (i2c_write_byte(buf[i]))                       goto fail;
    i2c_stop();
    return 0;

fail:
    i2c_stop();
    return -1;
}

/*-----------------------------------------------------------------------
 * Display geometry -- adjust HEIGHT for a 128x64 panel.
 *-----------------------------------------------------------------------*/
#define SH1107_WIDTH    128
#define SH1107_HEIGHT   128
#define SH1107_PAGES    (SH1107_HEIGHT / 8)

/* Some 128x128 SH1107 modules need a display-offset / column-offset
 * to line up correctly (Adafruit's 128x128 boards use 0x60). If your
 * image is shifted or wrapped, try adjusting these two first. */
#define SH1107_DISPLAY_OFFSET   0x60u
#define SH1107_COLUMN_OFFSET    0u

/*-----------------------------------------------------------------------
 * Init
 *-----------------------------------------------------------------------*/

int sh1107_init(void)
{
    int err = 0;

    i2c_init();

    err |= sh1107_command(0xAE);              /* display off */
    err |= sh1107_command(0xDC);               /* set display start line */
    err |= sh1107_command(0x00);
    err |= sh1107_command(0x81);               /* set contrast */
    err |= sh1107_command(0x2F);
    err |= sh1107_command(0x20);               /* memory addressing: page mode */
    err |= sh1107_command(0xA0);               /* segment remap: normal */
    err |= sh1107_command(0xC0);               /* COM scan direction: normal */
    err |= sh1107_command(0xA8);               /* set multiplex ratio */
    err |= sh1107_command((uint8_t)(SH1107_HEIGHT - 1));
    err |= sh1107_command(0xD3);               /* set display offset */
    err |= sh1107_command(SH1107_DISPLAY_OFFSET);
    err |= sh1107_command(0xD5);               /* set display clock divide */
    err |= sh1107_command(0x51);
    err |= sh1107_command(0xD9);               /* set pre-charge period */
    err |= sh1107_command(0x22);
    err |= sh1107_command(0xDB);               /* set VCOM deselect level */
    err |= sh1107_command(0x35);
    err |= sh1107_command(0xAD);               /* set DC-DC (charge pump) */
    err |= sh1107_command(0x8A);
    err |= sh1107_command(0xA6);               /* normal (not inverted) display */
    err |= sh1107_command(0xA4);               /* resume to RAM content display */

    err |= sh1107_command(0xAF);               /* display on */

    return err ? -1 : 0;                       /* -1 = no ACK somewhere -- check wiring/address */
}

/*-----------------------------------------------------------------------
 * Page/column addressing + framebuffer
 *-----------------------------------------------------------------------*/

static void sh1107_set_page(uint8_t page)
{
    sh1107_command((uint8_t)(0xB0 | (page & 0x0Fu)));
}

static void sh1107_set_column(uint8_t col)
{
    uint8_t c = (uint8_t)(col + SH1107_COLUMN_OFFSET);
    sh1107_command((uint8_t)(0x00 | (c & 0x0Fu)));         /* column low nibble */
    sh1107_command((uint8_t)(0x10 | ((c >> 4) & 0x0Fu)));  /* column high nibble */
}

static uint8_t framebuffer[SH1107_PAGES][SH1107_WIDTH];

void sh1107_clear(void)
{
    int p, c;

    for (p = 0; p < SH1107_PAGES; p++)
        for (c = 0; c < SH1107_WIDTH; c++)
            framebuffer[p][c] = 0;
}

/* color: 0 = pixel off, non-zero = pixel on. (0,0) is top-left. */
void sh1107_set_pixel(int x, int y, int color)
{
    int page, bit;

    if (x < 0 || x >= SH1107_WIDTH || y < 0 || y >= SH1107_HEIGHT)
        return;

    page = y / 8;
    bit  = y % 8;

    if (color)
        framebuffer[page][x] |= (uint8_t)(1u << bit);
    else
        framebuffer[page][x] &= (uint8_t)~(1u << bit);
}

/* Pushes the whole framebuffer to the display, one page at a time. */
void sh1107_display(void)
{
    int p;

    for (p = 0; p < SH1107_PAGES; p++) {
        sh1107_set_page((uint8_t)p);
        sh1107_set_column(0);
        sh1107_data_burst(framebuffer[p], SH1107_WIDTH);
    }
}

#ifdef SH1107_DEMO
#include <stdio.h>

static void draw_rect(int x0, int y0, int x1, int y1)
{
    int x, y;

    for (x = x0; x <= x1; x++) {
        sh1107_set_pixel(x, y0, 1);
        sh1107_set_pixel(x, y1, 1);
    }
    for (y = y0; y <= y1; y++) {
        sh1107_set_pixel(x0, y, 1);
        sh1107_set_pixel(x1, y, 1);
    }
}

static void draw_diagonal(void)
{
    int i, n;

    n = (SH1107_WIDTH < SH1107_HEIGHT) ? SH1107_WIDTH : SH1107_HEIGHT;
    for (i = 0; i < n; i++)
        sh1107_set_pixel(i, i, 1);
}

int main(void)
{
    if (sh1107_init() < 0) {
        kprintf("SH1107: no ACK from device -- check address/wiring/pull-ups\n");
        return 1;
    }

    sh1107_clear();
    draw_rect(0, 0, SH1107_WIDTH - 1, SH1107_HEIGHT - 1);
    draw_diagonal();
    sh1107_display();

    kprintf("SH1107: init and frame write complete\n");
    return 0;
}

#endif /* SH1107_DEMO */