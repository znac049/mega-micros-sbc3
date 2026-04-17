#include <stdio.h>
#include <ctype.h>
#include <machine.h>

#include "newmon.h"

ISR unhandled_exception(void) {
    set_led(5);

    while (1) {
    }
}

void set_default_vectors(void) {
    for (int i=0; i<VECTOR_TABLE_SIZE; i++) {
        set_isr_handler(i, (unsigned int)unhandled_exception);
    }
}