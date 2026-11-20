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
#include <bios.h>

typedef short lock_state_t;

#define HALT()			    __asm volatile("stop #0x2700\n")

/*
 * Every macro that writes SR is volatile and clobbers "memory", so the
 * compiler can't move loads or stores across it: an interrupt can't see
 * a half-finished update made inside LOCK()/UNLOCK().
 */
#define DISABLE_IRQS()      __asm__ volatile("or.w\t#0x0700,%%sr" : : : "memory", "cc")
#define ENABLE_IRQS()       __asm__ volatile("and.w\t#0xF8FF,%%sr" : : : "memory", "cc")

#define TRACE_ON()          __asm__ volatile("or.w\t#0x8000,%%sr" : : : "memory", "cc")
#define TRACE_OFF()         __asm__ volatile("and.w\t#0x7FFF,%%sr" : : : "memory", "cc")

#define NOP()               __asm volatile("nop\n")

#define SAVE_STATUS(saved) do {                                        \
    __asm__ volatile("move.w\t%%sr,%0" : "=dm" (saved) : : "memory");   \
} while (0)

// Save SR in saved (a lock_state_t), then mask all interrupts
#define LOCK(saved) do {                                               \
    __asm__ volatile("move.w\t%%sr,%0\n\tor.w\t#0x0700,%%sr"           \
                     : "=dm" (saved) : : "memory", "cc");              \
} while (0)

// Put SR back as LOCK() found it
#define UNLOCK(saved) do {                                             \
    __asm__ volatile("move.w\t%0,%%sr" : : "dm" (saved) : "memory", "cc"); \
} while (0)

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


// traps.asm
long trap0(long syscall_num, long arg1, long arg2, long arg3);


// vectors.c
unsigned int get_isr_handler(int vector_number);
unsigned int set_isr_handler(int vector_number, unsigned int isr);

