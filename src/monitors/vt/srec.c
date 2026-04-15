#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <machine.h>

#include "cli.h"

#define SREC_OK             0
#define SREC_TOOSHORT       1
#define SREC_BADFORMAT      2
#define SREC_UNIMPLEMENTED  3
#define SREC_CHECKSUM       4

static uint8_t checksum;

static int get_hex_byte(void) {
    uint8_t c1, c2;

    c1 = _getchar();
    c2 = _getchar();

    if (is_hex_char(c1) && is_hex_char(c2)) {
        uint8_t res = (hexval(c1)<<4) | hexval(c2);
        checksum += res;

        return (int)res;
    }

    return -1;
}

static int get_hex_bytes(int num_bytes) {
    int res = 0;
    int nibble;

    for (int i=0; i<num_bytes; i++) {
        nibble = get_hex_byte();

        if (nibble == -1) {
            return -1;
        }

        res = (res << 4) | nibble;
    }

    return res;
}

int read_data(uint32_t address, uint8_t num_bytes) {
    uint8_t *mem = (uint8_t *)address;
    uint8_t val;

    for (int i=0; i<num_bytes; i++) {
        val = get_hex_byte();
        *mem++ = val;
    }

    return 0;
}

int do_load_srec(int argc, char **argv) {
    char c;
    uint8_t byte_count;
    uint32_t address=0;
    uint32_t num_records = 0;
    uint32_t rx_num_records;
    uint8_t rx_checksum;
    uint8_t computed_checksum;
    uint8_t loading = 1;

    printf("Waiting for S-Records..\n");
    while (loading) {
        checksum = 0;
        c = _getchar();
        printf("c=%02x\n", c);
        while ((c == ' ') || (c == '\t')) {
            c = _getchar();
            printf("c=%02x\n", c);
        }

        printf("c=%02x\n", c);

        if (c != 'S') {
            printf("\nBadly formatted data.\n");
            return -1;
        }

        c = _getchar();
        switch (c) {
            case '1':
                byte_count = get_hex_byte();
                address = get_hex_bytes(2);
                read_data(address, byte_count);
                break;

            case '2':
                byte_count = get_hex_byte();
                address = get_hex_bytes(3);
                read_data(address, byte_count);
                break;

            case '3':
                byte_count = get_hex_byte();
                address = get_hex_bytes(4);
                read_data(address, byte_count);
                break;

            case '7':
                byte_count = get_hex_byte();
                address = get_hex_bytes(4);
                loading = 0;
                break;

            case '8':
                byte_count = get_hex_byte();
                address = get_hex_bytes(3);
                loading = 0;
                break;

            case '9':
                byte_count = get_hex_byte();
                address = get_hex_bytes(2);
                loading = 0;
                break;

            case '0':
                byte_count = get_hex_byte();
                address = get_hex_bytes(2);
                get_hex_bytes(byte_count-3);
                break;

            case '5':
                byte_count = get_hex_byte();
                rx_num_records = get_hex_bytes(2);
                printf("Num records apparently received: %d\n", rx_num_records);
                break;

            default:
                printf("\nBadly formatted S-Record.\n");
                return 1;
        }

        computed_checksum = checksum;
        rx_checksum = get_hex_byte();

        num_records++;

        printf("%08x  %02x  %02x\n", address, rx_checksum, computed_checksum);
    }

    return 0;
}