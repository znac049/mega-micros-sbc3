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


// bios_calls.c
int bios_exit(int exit_code);


// diasm_cmd.c
void handle_disasm_command(int argc, char *argv[]);


// dump.c
void dump(uint8_t *buf, size_t count, uint8_t print_zeroes, const char *heading, bool_t absolute_addresses);


// dump_cmd.c
void handle_dump_command(int argc, char *argv[]);


// elf.c
int load_elf(int fd);


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


// syscall.asm
unsigned int trap0_handler(int call_num, int arg1, int arg2, int arg3);
unsigned int trap14_handler(int call_num, int arg1, int arg2);


// usb_cmd.c
void handle_usb_command(int argc, char *argv[]);
