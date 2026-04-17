#include <stdio.h>
#include <machine.h>

uint8_t cpu_type = CPU_68000;

static const char *cpus[] = {
    "68000/68008",
    "68010",
    "68020",
    "68030 Enhanced 32-bit"
};

void pre_main(void) {
    printf("Mega-68030 Computer System\n");
    printf("C-Runtime initialising. Code is running in %s\n", running_in_rom?"ROM":"RAM");
    
    cpu_type = (uint8_t) detect_cpu_type();
    if (cpu_type > 3) {
        printf("UNKOWN processor type detected\n");
    }
    else {
        printf("%s Processor\n", cpus[cpu_type]);
    }
}