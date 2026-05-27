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

/* PI/T Timer Register Addresses */
#define pit_base ((volatile uint8_t*) 0xaf0001)

#define pit_pgcr  ((volatile uint8_t*) pit_base)
#define pit_psrr  ((volatile uint8_t*) pit_base+2)
#define pit_paddr ((volatile uint8_t*) pit_base+4)
#define pit_pbddr ((volatile uint8_t*) pit_base+6)
#define pit_pcddr ((volatile uint8_t*) pit_base+8)
#define pit_pivr  ((volatile uint8_t*) pit_base+10)
#define pit_pacr  ((volatile uint8_t*) pit_base+12)
#define pit_pbcr  ((volatile uint8_t*) pit_base+14)
#define pit_padr  ((volatile uint8_t*) pit_base+16)
#define pit_pbdr  ((volatile uint8_t*) pit_base+18)
#define pit_paar  ((volatile uint8_t*) pit_base+20)
#define pit_pbar  ((volatile uint8_t*) pit_base+22)
#define pit_pcdr  ((volatile uint8_t*) pit_base+24)
#define pit_psr   ((volatile uint8_t*) pit_base+26)

#define pit_tcr   ((volatile uint8_t*) pit_base+32)
#define pit_tivr  ((volatile uint8_t*) pit_base+34)
#define pit_cprh  ((volatile uint8_t*) pit_base+38)
#define pit_cprm  ((volatile uint8_t*) pit_base+40)
#define pit_cprl  ((volatile uint8_t*) pit_base+42)
#define pit_cntrh ((volatile uint8_t*) pit_base+46)
#define pit_cntrm ((volatile uint8_t*) pit_base+48)
#define pit_cntrl ((volatile uint8_t*) pit_base+50)
#define pit_tsr   ((volatile uint8_t*) pit_base+52)

uint32_t pit_get_counter(void);
uint32_t pit_set_counter(uint32_t);
uint32_t ticks(void);
void idle_for_ticks(uint32_t t);

void _claim_pit(void);
void _release_pit(void);

void pit_set_a(uint8_t val);
void pit_set_bits_a(uint8_t bits);
void pit_clear_bits_a(uint8_t bits);
void pit_set_b(uint8_t val);
void pit_set_bits_b(uint8_t bits);
void pit_clear_bits_b(uint8_t bits);
