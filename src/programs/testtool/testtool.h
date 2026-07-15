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

#define MAX_LINE    512
#define MAX_ARGS    32

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

// ds1307.c
extern int ds1307_read_time(ds1307_time_t *t);
extern int ds1307_read_nvram(int addr, uint8_t *buf, size_t num_bytes);
extern int ds1307_write_nvram(int addr, uint8_t *buf, size_t num_bytes);

// dump.c
extern void handle_dump_command(int argc, char *argv[]);

// eval.c.c
extern void handle_eval_command(int argc, char *argv[]);

// ext2.c
extern void handle_bginfo_subcommand(int argc, char *argv[]);
extern void handle_block_subcommand(int argc, char *argv[]);
extern void handle_inode_subcommand(int argc, char *argv[]);
extern void handle_sbinfo_subcommand(int argc);

// i2c.c
extern void i2c_init(void);
extern void i2c_start(void);
extern void i2c_stop(void);
extern int i2c_write_byte(uint8_t byte);
extern uint8_t i2c_read_byte(int nack);

// main.c
bool_t is_command(const char *cmd, const char *target, int min_target_len);

// rtc.c
extern void handle_rtc_command(int argc, char *argv[]);
extern void handle_rtc_subcommand(int argc, char *argv[]);

// show.c
extern void handle_show_command(int argc, char *argv[]);

// vars.c
extern void handle_vars_command(int argc);
