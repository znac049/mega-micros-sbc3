#include <stdio.h>
#include <machine.h>

/* 
 * LED numbers start at 0:
 *
 * 0-6 The bottom six LEDs in LD7 (connecteded to the duart)
 *
 * Note. All directions when looking at the board witrh the duart at the top.
 */

void set_led(int lednum) {
    if ((lednum >= 0) && (lednum < 6)) {
        *duart_opr_set = BIT((lednum+2));
    }
}

void clear_led(int lednum) {
    if ((lednum >= 0) && (lednum < 6)) {
        *duart_opr_reset = BIT((lednum+2));
    }
}
