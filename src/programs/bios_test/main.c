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

extern int t0(int sys_call_num, int p1, int p2, int p3);

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <machine.h>
#include <nonstd.h>

#include "bios_test.h"


static uint8_t bcd_to_dec(uint8_t bcd) {
    return (uint8_t)(((bcd >> 4) * 10) + (bcd & 0x0f));
}

static uint8_t dec_to_bcd(uint8_t dec) {
    return (uint8_t)(((dec / 10) << 4) | (dec % 10));
}

int ds1307_read(int addr, uint8_t *buf, size_t num_bytes) {
    size_t i;

    if ((addr < 0) || (addr > 63)) {
        return -1;
    }
 
    i2c_start();
    if (i2c_write_byte((uint8_t)(DS1307_ADDR << 1))) {
        i2c_stop();
        return -1;
    }

    if (i2c_write_byte(addr)) {
        i2c_stop();
        return -1;
    }
 
    i2c_start();
    if (i2c_write_byte((uint8_t)((DS1307_ADDR << 1) | 1))) {
        i2c_stop();
        return -1;
    }

    if (addr + num_bytes > 64) {
        num_bytes = 64 - addr;
    }
 
    for (i = 0; i < num_bytes; i++)
        *buf++ = i2c_read_byte(i == (num_bytes - 1));   // NACK the last byte
 
    i2c_stop();
 
    return num_bytes;
}
 
int ds1307_write(int addr, uint8_t *buf, size_t num_bytes) {
    size_t i;

    if ((addr < 0) || (addr > 63)) {
        return -1;
    }
 
    i2c_start();
    if (i2c_write_byte((uint8_t)(DS1307_ADDR << 1))) {
        i2c_stop();
        return -1;
    }

    if (i2c_write_byte(addr)) {
        i2c_stop();
        return -1;
    }
 
    for (i = 0; i < num_bytes; i++)
        i2c_write_byte(*buf++);
 
    i2c_stop();
 
    return num_bytes;
}
 
/* 
 * Reads the current time into *t.
 * Returns: 0  = success, oscillator running
 *          1  = success, but CH (clock halt) bit is set -- time is
 *               not advancing, the DS1307 needs its clock (re)started
 *         -1  = no ACK from the device (wrong address, wiring, or
 *               missing pull-ups)
 */
int ds1307_read_time(ds1307_time_t *t) {
    uint8_t raw[7];
    int num_read = ds1307_read(0, raw, 7);

    if (num_read != 7) {
        return -1;
    }
 
    t->seconds = bcd_to_dec(raw[0] & 0x7F);   /* mask off CH bit */
    t->minutes = bcd_to_dec(raw[1] & 0x7F);
    t->hours   = bcd_to_dec(raw[2] & 0x3F);   /* assumes 24-hour mode */
    t->day     = bcd_to_dec(raw[3] & 0x07);
    t->date    = bcd_to_dec(raw[4] & 0x3F);
    t->month   = bcd_to_dec(raw[5] & 0x1F);
    t->year    = bcd_to_dec(raw[6]);
 
    return (raw[0] & 0x80) ? 1 : 0;
}

int ds1307_write_time(ds1307_time_t *t) {
    uint8_t raw[7];
    int num_written;
    
    memcpy(raw, t, sizeof(ds1307_time_t));
    raw[0] = dec_to_bcd(t->seconds);
    raw[1] = dec_to_bcd(t->seconds);
    raw[2] = dec_to_bcd(t->seconds);
    raw[3] = dec_to_bcd(t->seconds);
    raw[4] = dec_to_bcd(t->seconds);
    raw[5] = dec_to_bcd(t->seconds);

    num_written = ds1307_write(0, raw, 7);

    if (num_written != 7) {
        return NOT_OK;
    }

    return OK;
}

int ds1307_read_nvram(int addr, uint8_t *buf, size_t num_bytes) {
    return ds1307_read(addr+8, buf, num_bytes);
}

int ds1307_write_nvram(int addr, uint8_t *buf, size_t num_bytes) {
    return ds1307_write(addr+8, buf, num_bytes);
}

static uint16_t check_address(volatile uint16_t *addr) {
    uint16_t orig = addr[0];
    uint16_t x,y;

    addr[0] = 0x5555;
    x = addr[0];

    addr[0] = 0xaaaa;
    y = addr[0];
    
    addr[0] = orig;

    return ((x == 0x5555) && (y == 0xaaaa));
}

/*
 * This isn't as simple as ayttempting to access possible RAM locations and
 * handling bes error to detect end of RAM as it looks like the CPLD generates
 * DTACK for the entire 8MB possible ram.
 */
uint32_t get_ram_end(void) {
    for (uint32_t addr=(ONE_MEG-1); addr<=RAM_MAX; addr+=ONE_MEG) {
        uint16_t res = check_address((uint16_t *)addr);

        // printf("%08x: %04x\n", addr, res);

        if (res == 0) {
            return addr;
        }
    }

    return 0x007fffff;
}

void pr_time(void) {
    ds1307_time_t t;
    int status;

    status = ds1307_read_time(&t);
    if (status < 0) {
        printf("DS1307: no ACK from device -- check address/wiring/pull-ups\n");
        return;
    }
    else if (status == 1) {
        printf("Warning: clock-halt (CH) bit is set -- oscillator is "
            "stopped, time above will not advance until the clock "
            "is (re)started. Time is standing still!\n");
    }
    
    // Display the time
    printf("Time: %02u:%02u:%02u\n", t.hours, t.minutes, t.seconds);
}

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

int do_oled(void)
{
    if (sh1107_init() < 0) {
        printf("SH1107: no ACK from device -- check address/wiring/pull-ups\n");
        return 1;
    }

    sh1107_clear();
    // draw_rect(0, 0, SH1107_WIDTH - 1, SH1107_HEIGHT - 1);
    // draw_diagonal();
    sh1107_pstr(0, 0, "SBC-3\n\nusb1@230400\nusb2@230400", NULL);
    sh1107_display();

    printf("SH1107: init and frame write complete\n");
    return 0;
}

int main(int argc, char *argv[]) {
    printf("Hello, Bob. Args are:\n");

    argv++;

    for (int i=0; i<argc; i++) {
        printf("%02d: '%s'\n", i, argv[i]);
    }

    // printf("RAM ends at %08x\n", get_ram_end());

    if (i2c_probe(DS1307_ADDR)) {
        printf("Found something on the i2c bus at %02x\n", DS1307_ADDR);
        pr_time();
    }

    if (i2c_probe(SH1107_ADDR)) {
        printf("Found something on the i2c bus at %02x\n", SH1107_ADDR);
        do_oled();
    }

    printf("\nPress ANY key to terminate...");
    while (!char_available()) {
        ;
    }

    printf("\nGot char %d\n", getchar());

    exit(0);
    return 42;
}