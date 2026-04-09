#include <stdio.h>
#include <machine.h>

#define LEDS1 0xa0
#define LEDS2 0x50

void twiddle_thumbs(void);

void scan() {
    static int lednum = 2;
    static int delta = 1;

    clear_led(lednum);

    lednum += delta;
    if ((lednum < 4) || (lednum > 9)) {
        delta = -delta;
        lednum += delta;
        lednum += delta;
    }

    set_led(lednum);
}

void dump_pit(void) {
    printf("           PGCR = $%02x\r\n", *pit_pgcr);
    printf("           PSRR = $%02x\r\n", *pit_psrr);
    printf("            TCR = $%02x\r\n", *pit_tcr);
    printf("            IVR = $%02x -> %03x\r\n", *pit_tivr, *pit_tivr<<2);
    printf("COUNTER PRELOAD = %08x\r\n", pit_get_counter());
    printf("        COUNTER = %08x\r\n", *pit_cntrh<<16 | *pit_cntrm<<8 | *pit_cntrl);
    printf("\r\n\n");
}

int main(void) {
    int zob = 0;

    printf("\r\nHere we go...\r\n");

    //dump_pit();

    // Plug in the isr

    printf("All done. Press any key to terminate...\r\n");
    while (!_char_available()) {
        printf("%08x", ticks());
        _putchar('\r');

        zob++;
        if (zob == 100) {
            scan();
            zob = 0;
        }
    }

    _getchar();

    //dump_pit();
}