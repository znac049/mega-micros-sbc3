#include "printf.h"

#define DUART_BASE 0xad0001
#define DUART_MR1A 0
#define DUART_MR2A 0
#define DUART_SRA 2
#define DUART_CSRA 2
#define DUART_CRA 4
#define DUART_RBA 6
#define DUART_TBA 6
#define DUART_IPCR 8
#define DUART_ACR 8
#define DUART_ISR 10
#define DUART_IMR 10
#define DUART_CUR 12
#define DUART_CTUR 12
#define DUART_CLR 14
#define DUART_CTLR 14
#define DUART_MR2B 16
#define DUART_MR2B 16
#define DUART_SRB 18
#define DUART_CSRB 18
#define DUART_CRB 20
#define DUART_RBB 22
#define DUART_TBB 22
#define DUART_IVR 24
#define DUART_OPCR 26
#define DUART_START_COUNTER 28
#define DUART_OPR_SET 28
#define DUART_STOP_COUNTER 30
#define DUART_OPR_RESET 30

#define DUART_RXRDY 0
#define DUART_TXRDY 2

#define LEDS1 0xa0
#define LEDS2 0x50

void twiddle_thumbs(void);

void init_duart(void) {
    volatile char *duart = (volatile char *)DUART_BASE;

    duart[DUART_IMR] = 0;
}

char _getchar(void) {
    volatile char *duart = (volatile char *)DUART_BASE;
    volatile char sr = duart[DUART_SRA];

    while (!(sr & 0x01)) {
        sr = duart[DUART_SRA];
    }

    return duart[DUART_RBA];
}

void _putchar(char c) {
    volatile char *duart = (volatile char *)DUART_BASE;
    volatile char sr = duart[DUART_SRA];

    while (!(sr & 0x08)) {
        sr = duart[DUART_SRA];
    }
    duart[DUART_TBA] = c;
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

    putstr("All done. Press any key to terminate...");
    _getchar();
}