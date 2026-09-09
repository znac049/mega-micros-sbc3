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

#include <ctype.h>
#include <ext2.h>
#include <filesystems.h>
#include <duart.h>
#include <pit.h>
#include <cf.h>
#include <vectors.h>
#include <blockdev.h>
#include <disk.h>
#include <fs.h>
#include <bios.h>

typedef short lock_state_t;

#define HALT()			    __asm volatile("stop #0x2700\n")

#define DISABLE_IRQS()		__asm volatile("or.w	#0x0700, %sr");
#define ENABLE_IRQS()		__asm volatile("and.w	#0xF8FF, %sr");

#define TRACE_ON()		    __asm volatile("or.w	#0x8000, %sr");
#define TRACE_OFF()		    __asm volatile("and.w	#0x7FFF, %sr");

#define NOP()               __asm volatile("nop\n")

#define SAVE_STATUS(saved) {				\
	__asm("move.w	%%sr, %0\n" : "=dm" ((saved)));	\
}

// i2c
#define DS1307_ADDR   0x68
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


#if 0
#define RESTORE_STATUS(saved) {					\
	__asm("move.w	%0, %%sr\n" : : "dm" ((saved)) :);	\
}
#endif

#define LOCK(saved) {					\
	__asm("move.w	%%sr, %0\n" : "=dm" ((saved)));	\
	DISABLE_IRQS();					\
}

#define UNLOCK(saved) {						\
	__asm("move.w	%0, %%sr\n" : : "dm" ((saved)) :);	\
}

#define ISR void __attribute((interrupt))

#define VEC_BUS_ERROR       2
#define VEC_ADDRESS_ERROR   3
#define VEC_ILLEGAL_INST    4
#define VEC_DIV0            5
#define VEC_CHK             6
#define VEC_TRAPV           7
#define BIT(n) (1<<n)

#define CPU_68000 0
#define CPU_68010 1
#define CPU_68020 2
#define CPU_68030 3

extern uint8_t running_in_rom;
extern uint8_t cpu_type;
extern volatile uint8_t bus_error_flag;


struct ds1307_time {
    uint8_t seconds;   /* 0-59 */
    uint8_t minutes;   /* 0-59 */
    uint8_t hours;     /* 0-23 (24-hour mode assumed) */
    uint8_t day;       /* 1-7, day of week (chip-defined numbering) */
    uint8_t date;      /* 1-31 */
    uint8_t month;     /* 1-12 */
    uint8_t year;      /* 0-99, add 2000 */
};

typedef struct ds1307_time ds1307_time_t;


struct font {
    const uint8_t width;
    const uint8_t height;
    const uint16_t *font_chars;
    const uint8_t *char_widths;
};

typedef struct font font_t;


int detect_cpu_type(void);
int measure_cpu_clock(void);

// block_devices.c
int bd_init(void);
int bd_read(block_device_t *dev, uint32_t block_num, uint8_t *buff, uint8_t subdev);


//block_devices/cf_block.c
int create_cf_dev(block_device_t *dev);


//block_devices/rom_block.c
int create_rom_dev(block_device_t *dev);


// ds1307.c
int ds1307_read(int addr, uint8_t *buf, size_t num_bytes);
int ds1307_write(int addr, uint8_t *buf, size_t num_bytes);
int ds1307_read_time(ds1307_time_t *t);
int ds1307_write_time(ds1307_time_t *t);
int ds1307_read_nvram(int addr, uint8_t *buf, size_t num_bytes);
int ds1307_write_nvram(int addr, uint8_t *buf, size_t num_bytes);



// filesystems.c
#if defined(BAREMETAL)

int vfs_init(void);
int vfs_shutdown(void);

int bios_chdir(const char *path);
int bios_getcwd(char*buff, size_t size);
vfile_t *bios_locate(const char *path);
int bios_creat(const char *pathname, mode_t mode);
int bios_open(const char *pathname, int flags);
int bios_close(int fd);
int bios_read(int fd, char *buff, size_t num_bytes);
size_t bios_write(int fd, const char *buff, size_t num_bytes);
int bios_opendir(const char *name);
ssize_t bios_getdents(int fd, void *dirp, size_t count);

#endif

// i2c.c
void i2c_init(void);
void i2c_start(void);
void i2c_stop(void);
int i2c_speed(int kbs);
uint8_t i2c_read_byte(int nack);
int i2c_write_byte(uint8_t byte);
int i2c_probe(uint8_t addr7);


// leds.c
void clear_led(int);
void set_led(int);


// safeio.c
int peek(volatile uint8_t *addr);
int poke(volatile uint8_t *addr, uint8_t val);


// sh1107.c
int sh1107_init(void);
void sh1107_clear(void);
void sh1107_set_pixel(int x, int y, int color);
int sh1107_pch(int x, int y, char c, font_t *font);
void sh1107_pstr(int x, int y, char *str, font_t *font);
void sh1107_display(void);


// traps.asm
long trap0(long syscall_num, long arg1, long arg2, long arg3);
