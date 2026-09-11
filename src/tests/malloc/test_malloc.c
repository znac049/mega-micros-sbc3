#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUM_MALLOCS 32

static int rnd(int max) {
    return rand() % max;
}

static void shuffle(int *order) {
    for (int i=0; i<500; i++) {
        int off1 = rnd(NUM_MALLOCS-1);
        int off2 = rnd(NUM_MALLOCS-1);
        int tmp = order[off1];

        order[off1] = order[off2];
        order[off2] = tmp;
    }

    printf("Shuffled to: ");
    for (int i=0; i<NUM_MALLOCS; i++) {
        printf("%d. ", order[i]);
    }
}

void main(void) {
    char *mem[NUM_MALLOCS];
    int order[NUM_MALLOCS];

    printf("Allocating randon chunks of memory...\n");
    for (int i=0; i<NUM_MALLOCS; i++) {
        int size = rand() %2048;
        char *cp = malloc(size);

        printf("%d (%d->%08x) ", i, size, cp);
        order[i] = i;

        mem[i] = cp;
    }

    shuffle(order);

    printf("\nNow freeing it all...\n");
    for (int i=0; i<NUM_MALLOCS; i++) {
        printf("%d (%08x). ", order[i], mem[order[i]]);
        // free(mem[order[i]]);
    }

    printf("Done.\n");
}