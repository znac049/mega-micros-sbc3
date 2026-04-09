#include <stdio.h>
#include <pit.h>
#include <machine.h>

static unsigned int saved_pit_isr=0;
static unsigned int saved_pit_counter=0;

static unsigned int pit_ticks = 0;

ISR pit_isr_handler(void) {
    pit_ticks++;

    *pit_tsr = 1;
}

void _claim_pit(void) {
    uint8_t pit_ivr = *pit_tivr;

    INTSOFF();

    saved_pit_isr = get_isr_handler(pit_ivr);
    set_isr_handler(pit_ivr, (unsigned int)pit_isr_handler);

    saved_pit_counter = pit_set_counter(1000);
    *pit_tsr = 1;

    INTSON();
}

void _release_pit(void) {
    uint8_t pit_ivr = *pit_tivr;

    INTSOFF();

    set_isr_handler(pit_ivr, saved_pit_isr);
    pit_set_counter(saved_pit_counter);
    *pit_tsr = 1;

    INTSON();
}

uint32_t ticks(void) {
    return pit_ticks;
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