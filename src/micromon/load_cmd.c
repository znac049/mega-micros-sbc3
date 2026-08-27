/* 
MIT License

Copyright (c) 2026 Bob Green

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <machine.h>
#include <disk.h>
#include <ext2.h>
#include <extras.h>

#include "micromon.h"
#include "expr.h"

#define DONE 1

#define TICKS_PER_SECOND 1000

static int calculated_xsum = 0;

static inline int hexval(char c) {
    if (('0' <= c) && (c <= '9')) {
        return c - '0';
    }

    c = tolower(c);
    if (('a' <= c) && (c <= 'f')) {
        return c - 'a' + 10;
    }

    return -1;
}

static int get_hex_bytes(int num_bytes, bool_t *error) {
    int res = 0;
    int nibble;
    int byte = 0;

    if (error != NULL) {
        *error = NO;
    }

    if ((num_bytes < 0) || (num_bytes > 8)) {
        if (error != NULL) {
            *error = YES;
        }

        return 0;
    }

    for (int i=0; i<num_bytes; i++) {
        nibble = hexval(kgetchar());

        if (nibble == -1) {
            kprintf("ERROR\n");
            if (error != NULL) {
                *error = YES;
            }

            return 0;
        }

        if (i & 1) {
            byte = byte | nibble;
            res = (res << 8) | byte;

            calculated_xsum += byte;
        }
        else {
            byte = nibble << 4;
        }
    }

    return res;
}

int compare_xsums(void) {
    int calc = ~calculated_xsum & 0xff;
    int rx_xsum = get_hex_bytes(2, NULL);

    // kprintf("\nComparing checksums: %02x and %02x\n", calc, rx_xsum);

    return (calc == rx_xsum)?OK:NOT_OK;
}

static int read_data(uint8_t *address, size_t count) {
    bool_t error;

    for (size_t i=0; i<count; i++) {
        int val = get_hex_bytes(2, &error);

        if (error == YES) {
            return NOT_OK;
        }

        *address++ = (uint8_t)val;
    }

    return OK;
}

static int load_single_srec(void) {
    char c = kgetchar();
    int res = OK;
    size_t count;
    uint8_t *address;
    bool_t count_error = NO;
    bool_t address_error = NO;

    // kprintf("Is '%c' (%d) the start of a valid S-Record?\n", isalnum(c)?c:'.', c);

    if (c == '\n') {
        // possibly a dangling newline from TeraTerm
        c = kgetchar();
        // kprintf("Skipped newline, now we have '%c' (%d)\n", isalnum(c)?c:'.', c);
    }

    if (c != 'S') {
        // Skip the line

        // kprintf("Skipping garbage line\n");
        while (c != '\r') {
            c = kgetchar();
        }

        return NOT_OK;
    }

    calculated_xsum = 0;
    c = kgetchar();

    // kprintf("S-Record type is '%c' (%d)\n", isalnum(c)?c:'.', c);

    // We have a possible S-Record
    switch(c) {
        case '0':
            // Vendor specific header
            count = get_hex_bytes(2, &count_error);
            address = (uint8_t *)get_hex_bytes(4, &address_error);

            if (count_error || address_error || address != 0 || count < 3) {
                kprintf("\nProblem reading count/address part of record\n");
                res = NOT_OK;
            }
            else {
                kprintf("Filename: ");
                for (size_t i=0; i<count-3; i++) {
                    uint8_t b = (uint8_t)get_hex_bytes(2, NULL);
                    kputchar(isalnum(b)?b:'.');
                }

                if (compare_xsums() != OK) {
                    kprintf(" (Bad checksum)");
                }
                kputchar('\n');
            }
            break;

        case '1':
            // Data record - 16-bit address
            count = get_hex_bytes(2, &count_error);
            address = (uint8_t *)get_hex_bytes(4, &address_error);

            if (count_error || address_error || count < 3) {
                kprintf("\nProblem reading count/address part of record\n");
                res = NOT_OK;
            }
            else {
                kprintf("\r%04x", address);
                if (read_data(address, count-3) == OK) {
                    if (compare_xsums() != OK) {
                        kprintf(" (failed checksum)");
                        res = -1;
                    }
                }
                else {
                    res = NOT_OK;
                }
            }
            break;

        case '2':
            // Data record - 24-bit address
            count = get_hex_bytes(2, &count_error);
            address = (uint8_t *)get_hex_bytes(6, &address_error);
            
            if (count_error || address_error || count < 4) {
                kprintf("\nProblem reading count/address part of record\n");
                res = NOT_OK;
            }
            else {
                kprintf("\r%06x", address);
                if (read_data(address, count-4) == OK) {
                    if (compare_xsums() != OK) {
                        kprintf(" (failed checksum)");
                        res = NOT_OK;
                    }
                }
                else {
                    res = NOT_OK;
                }
            }
            break;

        case '3':
            // Data record - 32-bit address
            count = get_hex_bytes(2, &count_error);
            address = (uint8_t *)get_hex_bytes(8, &address_error);
            
            if (count_error || address_error || count < 5) {
                kprintf("\nProblem reading count/address part of record\n");
                res = NOT_OK;
            }
            else {
                kprintf("\r%08x", address);
                if (read_data(address, count-5) == OK) {
                    if (compare_xsums() != OK) {
                        kprintf(" (failed checksum)");
                        res = NOT_OK;
                    }
                }
                else {
                    res = NOT_OK;
                }
            }
            break;

        case '5':
            // Record count
            count = get_hex_bytes(2, &count_error);
            address = (uint8_t *)get_hex_bytes(8, &address_error);

            if (count_error || address_error || count != 3) {
                kprintf("\nProblem reading count/address part of record\n");
                res = NOT_OK;
            }
            else {
                kprintf("\nRecord count is: %d", address);
                if (compare_xsums() != OK) {
                    kprintf(" (bad checksum)");
                    res = NOT_OK;
                }
                kputchar('\n');
            }
            break;

        case '9':
            // Termination record - 16-bit address
            count = get_hex_bytes(2, &count_error);
            address = (uint8_t *)get_hex_bytes(4, &address_error);
            
            if (count_error || address_error || count < 3) {
                kprintf("\nProblem with count/address part of record\n");
                res = NOT_OK;
            }
            else {
                if (compare_xsums() != OK) {
                    res = NOT_OK;
                }
                else {
                    kprintf("\nGO address is 0x%04x\n", address);
                    go_address = (uint32_t) address;
                }
            }
            
            res = DONE;
            break;

        case '8':
            // Termination record - 24-bit address
            count = get_hex_bytes(2, &count_error);
            address = (uint8_t *)get_hex_bytes(6, &address_error);
            
            if (count_error || address_error || count < 4) {
                kprintf("\nProblem with count/address part of record\n");
                res = NOT_OK;
            }
            else {
                if (compare_xsums() != OK) {
                    res = NOT_OK;
                }
                else {
                    kprintf("\nGO address is 0x%06x\n", address);
                    go_address = (uint32_t)address;
                }
            }
            
            res = DONE;
            break;

        case '7':
            // Termination record - 32-bit address
            count = get_hex_bytes(2, &count_error);
            address = (uint8_t *)get_hex_bytes(8, &address_error);
            
            if (count_error || address_error || count < 5) {
                kprintf("\nProblem with count/address part of record\n");
                res = NOT_OK;
            }
            else {
                if (compare_xsums() != OK) {
                    res = NOT_OK;
                }
                else {
                    kprintf("\nGO address is 0x%08x\n", address);
                    go_address = (uint32_t)address;
                }
            }

            res = DONE;
            break;
    }

    c = kgetchar();

    // Skip any remaining chars
    while (c != '\r') {
        if (c == '\r') {
            return res;
        }
        else if (!isspace(c)) {
            res = NOT_OK;
        }

        c = kgetchar();
    }

    return res;
}

static int wait_for_char_available(uint32_t seconds) {
    uint32_t now = reset_ticks();
    uint32_t target = now + TICKS_PER_SECOND;
    uint32_t timed_out = now + (seconds*TICKS_PER_SECOND);

    // kprintf("%d - %d - %d\n", now, target, timed_out);

    kprintf("%3d ", seconds);

    while (!kchar_available()) {
        now = ticks();

        if (now > timed_out) {
            return NOT_OK;
        }
        else if (now > target) {
            kputchar(BS);
            kputchar(BS);
            kputchar(BS);
            kputchar(BS);
            // kputchar(BELL);
            kprintf("%3d ", --seconds);

            target += TICKS_PER_SECOND;
        }
    }

    return OK;
}

void handle_load_command(int argc, char *argv[]) {
    int status;

    argc++;
    argv[0][0] = EOS;

    kprintf("Waiting for S-Records...");

    // Wait for serial data to arrive within 60 seconds
    if (wait_for_char_available(60) != OK) {
        kprintf("\nTimed out after 60 seconds\n");
        return;
    }

    kprintf("Reading...\n");
    status = load_single_srec();
    while (status != DONE) {
        status = load_single_srec();
    }

    kprintf("Data loaded ok\n");
}