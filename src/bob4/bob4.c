#include "printf.h"
#include "duart.h"

#define LEDS1 0xa0
#define LEDS2 0x50

static int x[4];

void twiddle_thumbs(void);

void init_duart(void) {
    volatile char *duart = (volatile char *)DUART_BASE;

    duart[DUART_IMR] = 0;
}

void putstr(char *str) {
    while (*str) {
        _putchar(*str++);
    }
}

int main(void) {

    volatile char *duart = (volatile char *)DUART_BASE;

    init_duart();
    putstr("Here we go...\r\n");

    for (int i=0; i<20; i++) {
        printf("Loop=%d\r\n", i);
        duart[DUART_OPR_SET] = (char) LEDS1;
        duart[DUART_OPR_RESET] = (char) LEDS2;
        twiddle_thumbs();

        duart[DUART_OPR_SET] = (char) LEDS2;
        duart[DUART_OPR_RESET] = (char) LEDS1;
        twiddle_thumbs();
    }

    x[3]=42;

    putstr("All done. Press any key to terminate...");
    _getchar();
}