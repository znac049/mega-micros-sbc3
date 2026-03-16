#include "printf.h"
#include "duart.h"

#define LEDS1 0xa0
#define LEDS2 0x50

void twiddle_thumbs(void);

static volatile char *duart = (char *)DUART_BASE;
static volatile char *pit = (char *)0xaf0001;
static int col=0;

void init_duart(void) {
    *duart_imr = 0;
    *duart_mr1a = 0x93;     /* RTS control 8-1-N*/
    *duart_mr1b = 0x07;

    *duart_acr  = 0x70;     /* Set 1 BRG */
    *duart_csra = 0xcc;     /* 38400 */
}

void scan() {
    static int lednum = 3;
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

void print_sep() {
    col++;
    if (col == 4) {
        col = 0;
        printf(" |\r\n| ");
    }
    else {
        printf(" |  ");
    }
}

void dump_vectors(void) {
    unsigned int *vectors = 0;
    char ivr;
    static char *names[] = {
        "Initial SSP",
        "Initial PC",
        "Bus Error",
        "Address Error",
        "Illegal Instruction",
        "Divide by 0",
        "CHK",
        "TRAPV",
        "Priv Violation",
        "Trace",
        "1010",
        "1111"
    };

    int i;
    char *format_str = "%-20s%03x: %08X";

    printf("| ");
    for (i=0; i<12; i++) {
        printf(format_str, names[i],  i<<2, vectors[i]);
        print_sep();
    }

#ifdef SHOW_RESERVED
    for (i=12; i<24; i++) {
        printf(format_str, "Reserved",  i<<2, vectors[i]);
        print_sep();
    }
#else
    printf("....\r\n| ");
#endif

    printf(format_str, "Spurious",  24<<2, vectors[24]);
    print_sep();

    for (i=25; i<32; i++) {
        char name[32];
        snprintf(name, 32, "Level%d", i-24);
        printf(format_str, name, i<<2, vectors[i]);
        print_sep();
    }

    for (i=32; i<48; i++) {
        char name[32];
        snprintf(name, 32, "Trap #%d", i-32);
        printf(format_str, name, i<<2, vectors[i]);
        print_sep();
    }

#ifdef SHOW_RESERVED
    for (i=48; i<64; i++) {
        printf(format_str, "Reserved",  i<<2, vectors[i]);
        print_sep();
    }
#else
    printf("....\r\n| ");
#endif

    for (i=64; i<256; i++) {
        printf(format_str, "", i<<2, vectors[i]);
        print_sep();
    }

    ivr = duart[DUART_IVR];
    printf("\r\nduart IVR reg = 0x%02x - vector at 0x%04X\r\n", ivr, ivr<<2);

    ivr = pit[34];
    printf("\r\nPI/T  IVR reg = 0x%02x - vector at 0x%04X\r\n", ivr, ivr<<2);
}

int main(void) {
    init_duart();
    printf("\r\nHere we go...\r\n");


#if 0
    for (int i=0; i<4; i++) {
        printf("Loop=%d\r\n", i);
        duart[DUART_OPR_SET] = (char) LEDS1;
        duart[DUART_OPR_RESET] = (char) LEDS2;
        twiddle_thumbs();

        duart[DUART_OPR_SET] = (char) LEDS2;
        duart[DUART_OPR_RESET] = (char) LEDS1;
        twiddle_thumbs();
    }
#endif

    dump_vectors();

    printf("All done. Press any key to terminate...");
    while (!_char_available()) {
        twiddle_thumbs();
        scan();
    }

    _getchar();
}