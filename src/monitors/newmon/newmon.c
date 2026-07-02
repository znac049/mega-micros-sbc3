#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <machine.h>

#include "newmon.h"

extern unsigned int _end;

static const char *cpus[] = {
    "68000/68008",
    "68010",
    "68020",
    "68030 Enhanced 32-bit"
};

unsigned int end_of_ram = 0;
static int bus_error_occurred = 0;

#define MEMORY_INC 4096     // Must be a power of 2

#define POKE(addr, val) *(uint8_t *)addr = val

// Handle bus error while we're checking memory
static ISR bus_error(void) {
    bus_error_occurred = 1;
}

static void banner(void) {
    printf("%c[2J", 27);
    printf("%c[H");
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

    printf("RAM found at [ 0x00000000 - 0x%08x ]\n", end_of_ram);
}

static void probe_ram(void) {
    unsigned int addr = MEMORY_INC;
    unsigned int old_handler = set_isr_handler(VEC_BUS_ERROR, (unsigned int)bus_error);

    if (!running_in_rom) {
        // Need to adjust the start point
        addr = (_end + MEMORY_INC) & (MEMORY_INC-1);
    }
    
    while (!bus_error_occurred) {
        POKE(addr, 42);
        if (!bus_error_occurred)
            addr += MEMORY_INC;
    }

    while (bus_error_occurred) {
        if (addr == 0)
            return;

        bus_error_occurred = 0;
        addr--;
        POKE(addr, 42);
    }

    set_isr_handler(VEC_BUS_ERROR, old_handler);
    end_of_ram = addr;
}

static void setup(void) {
    _claim_duart(NO, NO);

    probe_ram();
    set_default_vectors();

    _claim_pit();
    _init_heap();
}

void main(void) {
    setup();

    banner();

    printf("OK, we have control...\n");

    while (!char_available()) {
        printf("%08x\r", ticks());
    }
    getchar();
    printf("All done!\n");
}