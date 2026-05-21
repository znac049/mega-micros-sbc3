#include <stdio.h>
#include <stdlib.h>
#include <machine.h>

uint8_t cpu_type = CPU_68000;

FILE *stdin;
FILE *stdout;
FILE *stderr;

static const char *cpus[] = {
    "68000/68008",
    "68010",
    "68020",
    "68030 Enhanced 32-bit"
};

static void init_streams(void) {
	//stdin = stdout = fopen("CON:", "a+");
	//stderr = fopen("AUX:", "w");
}

void pre_main(void) {
    _claim_pit();
    _claim_duart();
    _init_heap();

    init_streams();

    if (running_in_rom) {
        printf("%c[2J", 27);
        printf("%c[H", 27);
        printf("Mega-68030 Computer System\n");
        printf("Code is running in %s\n", running_in_rom?"ROM":"RAM");
        
        // Detect CPU variant
        cpu_type = (uint8_t) detect_cpu_type();
        if (cpu_type > 3) {
            printf("UNKOWN processor type detected\n");
        }
        else {
            printf("%s Processor\n", cpus[cpu_type]);
        }
    }
}

void post_main(int status) {
    if (status) {
        printf("exited with %d\n", status);
    }

    _release_duart();
    _release_pit();
}