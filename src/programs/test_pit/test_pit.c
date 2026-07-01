#include <stdio.h>
#include <ctype.h>
#include <machine.h>

#define DELAY_INC 125

void test_port(volatile uint8_t *port, char *name) {
    *port = 0;

    for (int i=0; i<8; i++) {
        uint8_t val = 1<<i;

        putchar('\n');

        printf("\n%s%d", name, i);
        while (!char_available()) {
            uint32_t t = ticks() + DELAY_INC;

            *port = val;
            while ((ticks() < t) && (!char_available())) {
                ;
            }

            *port = 0;
            t += DELAY_INC;
            while ((ticks() < t) && (!char_available())) {
                ;
            }
        }
        getchar();
    }
}

void main(void) {
    *pit_paddr = 0xff;  // All outputs
    *pit_pbddr = 0xff;  // All outputs

    test_port(pit_padr, "PA");
    test_port(pit_pbdr, "PB");
}