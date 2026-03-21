#include "duart.h"

void set_led(int lednum) {
    if ((lednum < 5) || (lednum > 10)) {
        return;
    }

    *duart_opr_set = 1<<(lednum - 2);
}

void clear_led(int lednum) {
    if ((lednum < 5) || (lednum > 10)) {
        return;
    }

    *duart_opr_reset = 1<<(lednum - 2);
}