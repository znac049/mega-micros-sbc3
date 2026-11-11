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

#include <setjmp.h>
#include <limits.h>

#include "blockdev.h"
#include "filesystems.h"
#include "expr.h"
#include "duart.h"
#include "cf.h"
#include "disk.h"
#include "acrtc.h"
#include "pit.h"
#include "i2c.h"
#include "sh1107.h"
#include "ds1307.h"

#define MAX_LINE 512
#define MAX_ARGS 32

#define PIT_VECTOR_NUMBER 68

#define DEFAULT_PROGRAM_NAME "s.rec"

enum cmd_type {ANY=0, COMMAND=1, ALIAS, PREFIX};
typedef enum cmd_type cmd_type_t;

enum arg_type {NONE, BOTH, STR, VEC};
typedef enum arg_type arg_type_t;

enum loc_size {BYTE=1, WORD, LONG};
typedef enum loc_size loc_size_t;


struct command {
    const char *command;
    int min_required;
    cmd_type_t type;
    union {
        void (*handler)();
        char *cmd;
    };
    arg_type_t args;
};

typedef struct command command_t;


// Link time variables
extern uint32_t _d_start, _data_load_start, _data_length;
extern uint32_t _pretext_start, _postinit_end;
extern uint32_t _code_start, _code_end;
extern uint32_t _rodata_start, _rodata_end;
extern uint32_t _bss_start, _bss_end;

extern char program_name[PATH_MAX];


extern uint32_t ram_end;



// mon_crt0.asm
uint32_t get_cacr(void);
uint32_t set_cacr(uint32_t cacr);

// bios_calls.c
int bios_exit(int exit_code, pid_t pid);
int bios_test(int p1, int p2, int p3);


// block_devices.c
int bd_init(void);
int bd_read(block_device_t *dev, uint32_t block_num, uint8_t *buff, uint8_t subdev);


//block_devices/cf_block.c
int create_cf_dev(block_device_t *dev);


//block_devices/rom_block.c
int create_rom_dev(block_device_t *dev);


// cache_cmd.c
void handle_dcache_cmd(int argc, char *argv[]);
void handle_icache_cmd(int argc, char *argv[]);


// cat_cmd.c
void handle_cat_command(int argc, char *argv[]);


// cpu.c
int measure_cpu_clock(void);

// detect.c
bool_t is_pit_present(void);
bool_t is_duart_present(void);
bool_t is_rtc_present(void);
bool_t is_oled_present(void);
bool_t is_acrtc_present(void);


// dir_cmd.c
void handle_cd_command(int argc, char *argv[]);
void handle_dir_command(int argc, char *argv[]);
void handle_pwd_command(void);


// diasm_cmd.c
void handle_disasm_command(int argc, char *argv[]);


// dump.c
void dump(void *vbuf, loc_size_t size, size_t count, uint8_t print_zeroes, const char *heading, bool_t absolute_addresses);


// dump_cmd.c
void handle_bytedump_command(int argc, char *argv[]);
void handle_worddump_command(int argc, char *argv[]);
void handle_longdump_command(int argc, char *argv[]);


// elf.c
int load_elf(int fd);


// eval_cmd.c
void handle_eval_command(char *cmdline);

// filesystems.c
int vfs_init(void);
int vfs_shutdown(void);

int bios_chdir(const char *path);
int bios_getcwd(char*buff, size_t size);
int bios_isatty(int fd);
vfile_t *bios_locate(const char *path);
int bios_creat(const char *pathname, mode_t mode);
int bios_open(const char *pathname, int flags);
int bios_close(int fd);
int bios_read(int fd, char *buff, size_t num_bytes);
size_t bios_write(int fd, const char *buff, size_t num_bytes);
int bios_opendir(const char *name);
ssize_t bios_getdents(int fd, void *dirp, size_t count);


// go_cmd.c
extern uint32_t go_address;
extern jmp_buf go_env;
void handle_go_command(int argc, char *argv[]);

// heap.c
void bios_free(void *ptr, pid_t pid);
void *bios_malloc(size_t size, pid_t pid);
void clean_heap(pid_t pid);
void init_heap(void);
// void _heap_print_free(void);


// io.c
int dbgf(const char *format, ...);
int printk(const char *format, ...);


// load.c
void handle_load_command(int argc, char *argv[]);


// main.c
bool_t is_command(const char *cmd, const char *target, int min_target_len, cmd_type_t type);
int parse_truth(char *val);

// memory.c
uint32_t get_ram_end(void);
int test_ram(uint32_t start_addr, uint32_t end_addr);


// probe_cmd.c
void handle_probe_command(int argc);


// rtc_cmd.c
void handle_rtc_command(int argc, char *argv[]);


// safeio.c
int peek(volatile uint8_t *addr);
int poke(volatile uint8_t *addr, uint8_t val);
int peekw(volatile uint16_t *addr);
int pokew(volatile uint16_t *addr, uint16_t val);


// serial_cmd.c
void handle_ser_command(int argc, char *argv[]);


// set_cmd.c
void handle_setbyte_cmd(char *cmdline);
void handle_setword_cmd(char *cmdline);
void handle_setlong_cmd(char *cmdline);


// setup.c
void setup(void);


// showmount_cmd.c
void handle_showmount_command(int argc, char *argv[]);


// syscall.asm
unsigned int trap0_handler(int call_num, int arg1, int arg2, int arg3);
unsigned int trap14_handler(int call_num, int arg1, int arg2);


