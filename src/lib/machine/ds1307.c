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
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <machine.h>
#include <extras.h>


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
    int year = t->year;
    
    //memcpy(raw, t, sizeof(ds1307_time_t));
    raw[0] = dec_to_bcd(t->seconds);
    raw[1] = dec_to_bcd(t->minutes);
    raw[2] = dec_to_bcd(t->hours);
    raw[3] = dec_to_bcd(t->day);
    raw[4] = dec_to_bcd(t->date);
    raw[5] = dec_to_bcd(t->month);

    while (year >= 100) {
        year -= 100;
    }
    raw[6] = dec_to_bcd(year);

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