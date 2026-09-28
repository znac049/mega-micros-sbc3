#include <stdio.h>
#include <unistd.h>
#include <machine.h>

int main(void) {
    __asm__ __volatile__
    (
        "trap #5\n"
    );
    return 0;
}