#include <stdio.h>
#include <machine.h>

#include "newmon.h"

void main(void) {
    _claim_duart();
    pre_main();

    set_default_vectors();

    _claim_pit();
    _init_heap();

    printf("OK, we have control...\n");

    while (!char_available()) {
        printf("%08x\r", ticks());
    }
    getchar();
    printf("All done!\n");
}