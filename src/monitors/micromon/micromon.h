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

#define MAX_LINE 512
#define MAX_ARGS 32

#define PIT_VECTOR_NUMBER 68

// I2C
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

struct command {
    const char *command;
    int min_required;
    void (*handler)();
};

typedef struct command command_t;


// Link time variables
extern uint32_t _d_start, _data_load_start, _data_length;
extern uint32_t _pretext_start, _postinit_end;
extern uint32_t _code_start, _code_end;
extern uint32_t _rodata_start, _rodata_end;
extern uint32_t _bss_start, _bss_end;



// detect.c
bool_t is_pit_present(void);
bool_t is_duart_present(void);
bool_t is_rtc_present(void);
bool_t is_oled_present(void);


// diasm_cmd.c
void handle_disasm_command(int argc, char *argv[]);


// ds1307.c
int ds1307_read(int addr, uint8_t *buf, size_t num_bytes);
int ds1307_write(int addr, uint8_t *buf, size_t num_bytes);
int ds1307_read_time(ds1307_time_t *t);
int ds1307_write_time(ds1307_time_t *t);
int ds1307_read_nvram(int addr, uint8_t *buf, size_t num_bytes);
int ds1307_write_nvram(int addr, uint8_t *buf, size_t num_bytes);


// dump.c
void dump(uint8_t *buf, size_t count, uint8_t print_zeroes, const char *heading, bool_t absolute_addresses);


// dump_cmd.c
void handle_dump_command(int argc, char *argv[]);


// eval_cmd.c
void handle_eval_command(int argc, char *argv[]);


// go_cmd.c
extern uint32_t go_address;
extern jmp_buf go_env;
void handle_go_command(int argc, char *argv[]);


// io.c
int dbgf(const char *format, ...);
int printk(const char *format, ...);


// load.c
void handle_load_command(int argc, char *argv[]);


// main.c
bool_t is_command(const char *cmd, const char *target, int min_target_len);


// memory.c
uint32_t get_ram_end(void);


// probe_cmd.c
void handle_probe_command(int argc);


// rtc_cmd.c
void handle_rtc_command(int argc, char *argv[]);


// setup.c
void setup(void);


// sh1107.c
int sh1107_init(void);
void sh1107_clear(void);
void sh1107_set_pixel(int x, int y, int color);
int sh1107_pch(int x, int y, char c, font_t *font);
void sh1107_pstr(int x, int y, char *str, font_t *font);
void sh1107_display(void);

// syscall.asm
unsigned int trap0_handler(int call_num, int arg1, int arg2, int arg3);
unsigned int trap14_handler(int call_num, int arg1, int arg2);


// usb_cmd.c
void handle_usb_command(int argc, char *argv[]);
