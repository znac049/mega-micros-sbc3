#include <stdio.h>
#include <stdlib.h>

#include "cli.h"

extern void invoke(uint32_t addr);

static unsigned int go_address = 0x280000;

int do_go(int argc, char **argv) {
    if (argc != 2) {
        printf("usage: go <address>\n");
        return -1;
    }

    go_address = strtol(argv[1], NULL, 10);
    printf("Passing control to $%08x\n", go_address);
    invoke(go_address);

    return 0;
}