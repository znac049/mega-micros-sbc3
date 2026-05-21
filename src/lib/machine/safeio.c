#include <ctype.h>
#include <machine.h>

static int result;

static ISR bus_error(void) {
    result = -1;
}

int peek(uint8_t *addr) {
    unsigned int old_handler = set_isr_handler(VEC_BUS_ERROR, (unsigned int)bus_error);
    uint8_t data;
    
    result = 0;
    data = *addr;

    set_isr_handler(VEC_BUS_ERROR, old_handler);
    
    return (result == -1)?result:(int)data;
}

int poke(uint8_t *addr, uint8_t val) {
    unsigned int old_handler = set_isr_handler(VEC_BUS_ERROR, (unsigned int)bus_error);

    result = val;
    *addr = val;

    set_isr_handler(VEC_BUS_ERROR, old_handler);
    
    return result;
}