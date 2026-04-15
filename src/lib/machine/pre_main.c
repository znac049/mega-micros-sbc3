#include <stdio.h>
#include <machine.h>

void pre_main(void) {
    printf("C-Runtime initialising. Code is running in %s\n", running_in_rom?"ROM":"RAM");
}