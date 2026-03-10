#define DUART_BASE 0xad0001
#define DUART_OPR_SET 28
#define DUART_OPR_RESET 30

#define LEDS1 0xa0
#define LEDS2 0x50

void twiddle_thumbs(void);

int main(void) {

    volatile char *duart = (volatile char *)DUART_BASE;

    for (int i=0; i<20; i++) {
        duart[DUART_OPR_SET] = (char) LEDS1;
        duart[DUART_OPR_RESET] = (char) LEDS2;
        twiddle_thumbs();

        duart[DUART_OPR_SET] = (char) LEDS2;
        duart[DUART_OPR_RESET] = (char) LEDS1;
        twiddle_thumbs();
    }
}