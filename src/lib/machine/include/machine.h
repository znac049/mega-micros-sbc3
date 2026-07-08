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
#include <duart.h>
#include <pit.h>
#include <cf.h>
#include <vectors.h>
#include <cb.h>
#include <disk.h>
#include <ext2.h>
#include <fs.h>

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

#define RESTORE_STATUS(saved) {					\
	__asm("move.w	%0, %%sr\n" : : "dm" ((saved)) :);	\
}

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


int detect_cpu_type(void);
int measure_cpu_clock(void);

int trap0(int num, int arg1, int arg2, int arg3);

int peek(uint8_t *addr);
int poke(uint8_t *addr, uint8_t val);
