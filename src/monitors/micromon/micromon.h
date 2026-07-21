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

#define MAX_LINE 512
#define MAX_ARGS 32

#define OK 0
#define NOT_OK -1

#define DUART_VECTOR_NUMBER 64
#define PIT_VECTOR_NUMBER 68

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


// go_cmd.c
extern uint32_t go_address;
void handle_go_command(int argc, char *argv[]);


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


// i2c.c
void i2c_init(void);
void i2c_start(void);
void i2c_stop(void);
int i2c_write_byte(uint8_t byte);
uint8_t i2c_read_byte(int nack);


// kio.c
void setup_duart(int is_xr, int clk_dbl);
bool_t kchar_available(void);
int kgetchar(void);
char *kgets(char *s);
int kputchar(int c);
int kputs(const char *s);
int kprintf(const char *format, ...);
int bios_getchar(int port);
int kgetchar(void);
int bios_putchar(int port, int c);
int kputchar(int c);
bool_t bios_char_available(int port);
char *bios_gets(int port, char *s);
int bios_puts(int port, const char *s);
int bios_printf(int port, const char *format, ...);
int bios_set_baud(int port, uint32_t baudrate);
int kio_rx_info(void);


// load.c
void handle_load_command(int argc, char *argv[]);


// main.c
bool_t is_command(const char *cmd, const char *target, int min_target_len);


// memory.c
uint32_t get_ram_size(void);


// probe_cmd.c
void handle_probe_command(int argc);


// rtc_cmd.c
void handle_rtc_command(int argc, char *argv[]);


// syscall.asm
unsigned int trap0_handler(int call_num, int arg1, int arg2);


// usb_cmd.c
void handle_usb_command(int argc, char *argv[]);
