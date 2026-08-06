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
#include <filesystems.h>
#include <duart.h>
#include <pit.h>
#include <cf.h>
#include <vectors.h>
#include <blockdev.h>
#include <disk.h>
#include <ext2.h>
#include <fs.h>
#include <syscalls.h>

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


// crt0.asm
int do_trap0(uint32_t syscall_num, uint32_t arg1, uint32_t arg2, uint32_t arg3);


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
int vfs_chdir(const char *path);
int vfs_getcwd(char*buff, size_t size);
vdir_t *vfs_locate(const char *path);
int vfs_creat(const char *pathname, mode_t mode);
int vfs_open(const char *pathname, int flags);
int vfs_close(int fd);
int vfs_read(int fd, char *buff, size_t num_bytes);
size_t vfs_write(int fd, const char *buff, size_t num_bytes);

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


// traps.c
long trap0(long syscall_num, long arg1, long arg2, long arg3);


#if defined(BAREMETAL)

// ext2...
// ext2/e2block.c
int ext2_read_block(ext2_fs_t *fs, uint32_t block_num, uint8_t *buffer);
int ext2_read_blocks(ext2_fs_t *fs, uint32_t block_num, int num_blocks, uint8_t *buffer);
int ext2_read_fs_block(ext2_fs_t *fs, uint32_t block_num);
int ext2_init_block_follower(ext2_fs_t *fs, uint32_t inode_num, ext2_block_follower_t *bf);
void ext2_reset_block_follower(ext2_block_follower_t *bf);
uint32_t ext2_get_next_block_num(ext2_block_follower_t *bf);

// ext2/e2dir.c
int ext2_closedir(ext2_dirp_t *dirp);
ext2_dirp_t *ext2_opendir(vmp_t *mp, vdir_t *dir, const char *name);
ext2_dirent_t *ext2_readdir(ext2_dirp_t *dirp);
void ext2_rewinddir(ext2_dirp_t *dirp);
vdir_t *ext2_locate(vmp_t *mp, vdir_t *dir, const char *pathname);

// ext2/e2dump.c
void dump_ext2_bg(ext2_bg_t *bg, int bg_num, ext2_sb_t *sb);
void dump_ext2_inode(ext2_inode_t *in, int in_num);
void dump_ext2_sb(ext2_sb_t *sb);

// ext2/e2endian.c
void ext2_sanitize_superblock(ext2_sb_t *src_sb, ext2_sb_t *dst_sb);
void ext2_sanitize_bg(ext2_bg_t *src_bg, ext2_bg_t *dst_bg);
void ext2_sanitize_inode(ext2_inode_t *src_in, ext2_inode_t *dst_in);
void ext2_sanitize_dirent(ext2_dirent_t *src_dp, ext2_dirent_t *dst_dp);

// ext2/ext2.c
ext2_bg_t *ext2_get_bg(ext2_fs_t *fs, uint32_t blockgroup_num);
int ext2_get_inode(ext2_fs_t *fs, uint32_t inode_num, ext2_inode_t *inode);
vmp_t *ext2_mount(vmp_t *mp);
int ext2_umount(vmp_t *mp);
int is_ext2(ext2_sb_t *sb);
bool_t ext2_has_superblock(uint32_t bg_num);
int setup_vfs_ext2_handler(vfs_fs_t *vfs);

// ext2/e2file.c
int ext2_file_reader(ext2_fs_t *fs, uint32_t inode_num);

// ext2/utils.c
// void printn(const char *pfx, const uint8_t *str, int len);

#endif
