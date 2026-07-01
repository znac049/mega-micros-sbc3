#include <stdio.h>
#include <string.h>
#include <machine.h>

extern void xr68681_putchar(int ch, uint8_t minor);

int zob(int a, int b) {
    return a + b;
}

void waitawhile(void) {
    for (int i=0; i<1000000; i++) {
        int z = zob(i, z);
        if (z)
            z++;
    }
}

int main(void) {
#if 0
    printf("Hello, world\n");

    idle_for_ticks(2000);

    printf("Press ENTER to terminate\n");
    getchar();

    printf("That's all folks...\n");

    for (int i=0; i<10000; i++) {
        putchar('x');
    }
#endif

    char *msg = "H\r\n";

    while (*msg) {
        xr68681_putchar(*msg++, 0);
    }

    return 42;
}