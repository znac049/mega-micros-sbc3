#include <stdio.h>
#include <machine.h>

#define LEDS1 0xa0
#define LEDS2 0x50

void twiddle_thumbs(void);

void scan() {
    static int lednum = -1;
    static int delta = 1;

    clear_led(lednum);

    lednum += delta;
    if ((lednum < 0) || (lednum >= 6)) {
        delta = -delta;
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
    int old_timer, timer;

    printf("\r\nHere we go...\r\n");

    printf("All done. Press any key to terminate...\r\n");
    timer = ticks();
    old_timer = 0;
    while (!char_available()) { 
        if (old_timer != timer) {
            printf("%08x", ticks());
            _putchar('\r');
        }

        old_timer = timer;
        timer = ticks();

        zob++;
        if (zob == 10000) {
            scan();
            zob = 0;
        }
    }
    
    printf("\nType a string followed by ENTER and we're done!\n");
    printf("Quit character was %d\n", getchar());
}