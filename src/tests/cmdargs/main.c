#include <stdio.h>
#include <unistd.h>
#include <machine.h>

int main(int argc, char *argv[]) {
    printf("Command line args:\n");

    for (int i=0; i<argc; i++) {
        printf("%d: '%s'\n", i, argv[i]);
    }

    printf("---\n");
    return 0;
}