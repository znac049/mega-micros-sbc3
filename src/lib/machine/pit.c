#include "pit.h"
#include "printf.h"

void _pit_reset(void) {
    
}

unsigned int _pit_get_counter(void) {
    unsigned int l, m, h;
    
    l = *pit_cprl;
    m = *pit_cprm;
    h = *pit_cprh;

    return (h<<16) | (m<<8) | l;
}

unsigned int _pit_set_counter(unsigned int count_max) {
    unsigned int old_count_max = _pit_get_counter();
    unsigned int l, m, h;

    l = count_max & 0xff;
    m = (count_max >> 8) & 0xff;
    h = (count_max >> 16) & 0xff;

    printf("_pit_set_counter(%08x): %02x %02x %02x\r\n", count_max, h, m, l);

    *pit_cprh = h;
    *pit_cprm = m;
    *pit_cprl = l;

    return old_count_max;
}