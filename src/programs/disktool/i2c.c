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
#include <disk.h>
#include <ext2.h>
#include <nonstd.h>

#include "disktool.h"

#define PIT_BASE   0xAF0001UL

#define PCDDR      (*(pit_pcddr))
#define PCDR       (*(pit_pcdr))

#define SDA_BIT    0
#define SCL_BIT    1

#define SDA_MASK   (1 << SDA_BIT)
#define SCL_MASK   (1 << SCL_BIT)

/* Bus timing: crude busy-wait. Calibrate against your actual bus
 * clock to hit a real I2C bit rate (100kHz/400kHz) -- measure with a
 * scope or logic analyzer rather than trusting this constant as-is. */
#define I2C_DELAY_COUNT   40

static void i2c_delay(void) {
    volatile int i;
    for (i = 0; i < I2C_DELAY_COUNT; i++)
        ;
}

static void sda_low(void) {
    PCDR  &= (uint8_t)~SDA_MASK;   /* output latch = 0 */
    PCDDR |= SDA_MASK;             /* drive it */
}

static void sda_release(void) {
    PCDDR &= (uint8_t)~SDA_MASK;   /* input: let pull-up take it high */
}

static void scl_low(void) {
    PCDR  &= (uint8_t)~SCL_MASK;
    PCDDR |= SCL_MASK;
}

static void scl_release(void) {
    PCDDR &= (uint8_t)~SCL_MASK;
    while ((PCDR & SCL_MASK) == 0)
        ;
}

static int sda_read(void) {
    return (PCDR & SDA_MASK) ? 1 : 0;
}

void i2c_init(void) {
    PCDDR &= (uint8_t)~(SDA_MASK | SCL_MASK);   /* both lines released */
}

void i2c_start(void) {
    sda_release();
    scl_release();
    i2c_delay();
    sda_low();
    i2c_delay();
    scl_low();
    i2c_delay();
}

void i2c_stop(void) {
    sda_low();
    i2c_delay();
    scl_release();
    i2c_delay();
    sda_release();
    i2c_delay();
}

/* Send one byte MSB-first. Returns 0 if the slave ACKed, 1 if NACKed. */
int i2c_write_byte(uint8_t byte) {
    int bit;
    int ack;

    for (bit = 7; bit >= 0; bit--) {
        scl_low();
        if (byte & (1 << bit))
            sda_release();
        else
            sda_low();
        i2c_delay();
        scl_release();
        i2c_delay();
    }

    scl_low();
    sda_release();          /* let the slave drive ACK/NACK */
    i2c_delay();
    scl_release();
    i2c_delay();
    ack = sda_read();       /* 0 = ACK, 1 = NACK */
    scl_low();

    return ack;
}

/* Read one byte MSB-first. Pass nack=0 to ACK (more bytes follow),
 * nack!=0 to NACK (use this on the last byte you want from the slave). */
uint8_t i2c_read_byte(int nack) {
    uint8_t value = 0;
    int bit;

    sda_release();

    for (bit = 7; bit >= 0; bit--) {
        scl_low();
        i2c_delay();
        scl_release();
        value = (uint8_t)((value << 1) | sda_read());
        i2c_delay();
    }

    scl_low();
    if (nack)
        sda_release();
    else
        sda_low();
    i2c_delay();
    scl_release();
    i2c_delay();
    scl_low();
    sda_release();

    return value;
}
