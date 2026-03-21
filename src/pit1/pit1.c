#include "printf.h"
#include "machine.h"

#define LEDS1 0xa0
#define LEDS2 0x50

static unsigned int ticks = 0;

void twiddle_thumbs(void);

void isr(void) {
    ticks++;

    *pit_tsr = 1;
    __asm(" rte");
}

void scan() {
    static int lednum = 2;
    static int delta = 1;

    clear_led(lednum);

    lednum += delta;
    if ((lednum < 2) || (lednum > 7)) {
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
    printf("COUNTER PRELOAD = %08x\r\n", _pit_get_counter());
    printf("        COUNTER = %08x\r\n", *pit_cntrh<<16 | *pit_cntrm<<8 | *pit_cntrl);
    printf("\r\n\n");
}

int main(void) {
    unsigned char ivr = *pit_tivr;

    unsigned int old_counter;
    unsigned int old_isr;

    unsigned int *xxx = _get_vectors_base();
    printf("zz=%08x\r\n", (unsigned int)xxx);

    int zob = 0;

    printf("\r\nHere we go...\r\n");

    dump_pit();

    // Plug in the isr

    old_counter =  _pit_set_counter(1000);
    *pit_tsr = 1;
    dump_pit();
    __asm(" move.w #0x2700,%sr");
    old_isr = get_isr_vector(ivr);
    set_isr_vector(ivr, isr);
    __asm(" move.w #0x2000,%sr");

    printf("Old counter was %d - old isr was $%08x\r\n", old_counter, old_isr);

    printf("All done. Press any key to terminate...\r\n");
    while (!_char_available()) {
        printf("%08x\r", ticks);

        zob++;
        if (zob == 100) {
            scan();
            zob = 0;
        }
    }

    _getchar();

    dump_pit();

    _pit_set_counter(old_counter);
    __asm(" move.w #0x2700,%sr");
    set_isr_vector(ivr, (void (*)(void))old_isr);
    __asm(" move.w #0x2000,%sr");
}