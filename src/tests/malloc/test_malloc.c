#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void main(void) {
    char *cp1, *cp2, *cp3, *cp4;

    cp1 = malloc(512);
    memset(cp1, 'x', 512);
    printf("cp1: %08x\n", cp1);

    cp2 = malloc(8192);
    memset(cp2, 'y', 8192);
    printf("cp2: %08x %d\n", cp2, cp1-cp2);

    cp3 = malloc(9);
    memset(cp3, 'z', 9);
    printf("cp3: %08x %d\n", cp3, cp2-cp3);

    cp4 = malloc(7);
    memset(cp4, '!', 7);
    printf("cp4: %08x %d\n", cp4, cp3-cp4);

    free(cp3);
    heap_print_free();

    free(cp1);
    heap_print_free();

    free(cp2);
    heap_print_free();

    free(cp4);
    heap_print_free();
}