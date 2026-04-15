#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "cli.h"

static int dump_address = 0x100000;
static int dump_len = 16;

int is_printable(char ch) {
	return ((ch >= ' ') && (ch <= '~'));
}

void dump_buffer(uint8_t *buffer, int buffer_len) {
	while (buffer_len) {
		printf("  : ");
	}
}

int do_dump(int argc, char **argv) {
    printf("Argc=%d\n", argc);

    if (argc > 3) {
        printf("usage: dump [<address> [<num_bytes>]]\n");
        return -1;
    }

    if (argc >= 2) {
        dump_address = strtol(argv[1], NULL, 10);

        if (argc == 3) {
            dump_len = strtol(argv[2], NULL, 10);
        }
    }

    dump_buffer((uint8_t *)dump_address, dump_len);
    dump_address += dump_len;

    return 0;
}