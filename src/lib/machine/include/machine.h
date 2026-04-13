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