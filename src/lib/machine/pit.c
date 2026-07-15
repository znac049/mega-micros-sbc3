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

#include <stdio.h>
#include <pit.h>
#include <machine.h>

static unsigned int saved_pit_isr=0;
static unsigned int saved_pit_counter=0;

static volatile unsigned int pit_ticks = 0;

static uint8_t pit_port_a = 0;
static uint8_t pit_port_b = 0;

ISR pit_isr_handler(void) {
    pit_ticks++;

    *pit_tsr = 1;
}

void _claim_pit(void) {
    uint8_t pit_ivr = *pit_tivr;

    DISABLE_IRQS();

    saved_pit_isr = get_isr_handler(pit_ivr);
    set_isr_handler(pit_ivr, (unsigned int)pit_isr_handler);

    // WIth a 10MHz clock, this will generate 1mS interrupts
    saved_pit_counter = pit_set_counter(10000/32);
    *pit_tcr = 0xa1;
    *pit_tsr = 1;   

    ENABLE_IRQS();
}

void _release_pit(void) {
    uint8_t pit_ivr = *pit_tivr;

    DISABLE_IRQS();

    set_isr_handler(pit_ivr, saved_pit_isr);
    pit_set_counter(saved_pit_counter);
    // *pit_tsr = 1;

    ENABLE_IRQS();
}

uint32_t ticks(void) {
    return pit_ticks;
}

uint32_t reset_ticks(void) {
    int saved_state;

    LOCK(saved_state);

    pit_ticks = 0;

    UNLOCK(saved_state);

    return pit_ticks;
}

void idle_for_ticks(uint32_t t) {
    uint32_t end_tick = pit_ticks + t;

    while (pit_ticks <= end_tick) {
        ;
    }
}

uint32_t pit_get_counter(void) {
    uint32_t l, m, h;
    
    l = *pit_cprl;
    m = *pit_cprm;
    h = *pit_cprh;

    return (h<<16) | (m<<8) | l;
}

uint32_t pit_set_counter(uint32_t count_max) {
    uint32_t old_count_max = pit_get_counter();
    uint32_t l, m, h;

    l = count_max & 0xff;
    m = (count_max >> 8) & 0xff;
    h = (count_max >> 16) & 0xff;

    //printf("pit_set_counter(%08x): %02x %02x %02x\r\n", count_max, h, m, l);

    *pit_cprh = h;
    *pit_cprm = m;
    *pit_cprl = l;

    return old_count_max;
}

void pit_set_a(uint8_t val) {
    pit_port_a = ~val;

    *pit_padr = pit_port_a;
}

void pit_set_bits_a(uint8_t bits) {
    pit_port_a = pit_port_a | ~bits;

    *pit_padr = pit_port_a;
}

void pit_clear_bits_a(uint8_t bits) {
    pit_port_a = pit_port_a & bits;

    *pit_padr = pit_port_a;
}

void pit_set_b(uint8_t val) {
    pit_port_b = ~val;

    *pit_pbdr = pit_port_b;
}

void pit_set_bits_b(uint8_t bits) {
    pit_port_b = pit_port_b | ~bits;

    *pit_pbdr = pit_port_b;
}

void pit_clear_bits_b(uint8_t bits) {
    pit_port_b = pit_port_b & bits;

    *pit_pbdr = pit_port_b;
}