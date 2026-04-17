#pragma once

#include <ctype.h>
#include <duart.h>
#include <pit.h>
#include <cf.h>
#include <vectors.h>
#include <cb.h>

#define INTSOFF() __asm("or.w #0x0700,%sr")
#define INTSON() __asm("and.w #0xf8ff,%sr")

#define ISR void __attribute((interrupt))

#define BIT(n) (1<<n)

#define CPU_68000 0
#define CPU_68010 1
#define CPU_68020 2
#define CPU_68030 3

extern uint8_t running_in_rom;
extern uint8_t cpu_type;


extern int detect_cpu_type(void);