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

#include <ctype.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <nonstd.h>
#include <machine.h>

#include "micromon.h"
#include "expr.h"


#define PAD_COL 45

static int major = 0;
static int minor = 3;
static int MAGIC_BUILD_NUMBER = 215;

// Link time variables
extern uint32_t _d_start, _data_load_start, _data_length;
extern uint32_t _pretext_start, _postinit_end;
extern uint32_t _code_start, _code_end;
extern uint32_t _rodata_start, _rodata_end;
extern uint32_t _bss_start, _bss_end;


void handle_zob_command(int argc, char *argv[]);
void print_help(int argc, char *argv[]);

static command_t commands[] = {
    {"help",        2, print_help},
    {"disassemble", 3, handle_disasm_command},
    {"dump",        2, handle_dump_command},
    {"eval",        2, handle_eval_command},
    {"go",          0, handle_go_command},
    {"load",        2, handle_load_command},
    {"probe",       3, handle_probe_command},
    {"rtc",         3, handle_rtc_command},
    {"usb1",        0, handle_usb_command},
    {"usb2",        0, handle_usb_command},
    {"zob",         0, handle_zob_command},
};

#define NUM_COMMANDS (sizeof(commands)/sizeof(command_t))

static char *jumper_txt[6] = {"TxA EN", "TxB EN", "XR EN", "EMU Boot", "DIAG EN", "ACRTC MODE"};

static void pr_section(const char *name, uint32_t start, uint32_t end) {
    kprintf("%15s 0x%06x   0x%06x   %d\n", name, start, end, end-start);
}

static void padstr(const char *s, int cols) {
    int l = strlen(s);

    if (l > cols) {
        l = cols;
    }

    for (int i=0; i<l; i++) {
        kputchar(s[i]);
    }

    for (int i=l; i<cols; i++) {
        kputchar('.');
    }
}

void setup(void) {
    uint8_t *ram_end = (uint8_t *)0x3fffff; //get_ram_size();
    uint8_t jumpers;
    int is_xr;
    int list_started = NO;

    *pit_tivr = PIT_VECTOR_NUMBER;
    _claim_pit();

    // Set PIT ports A and B as outputs
    *pit_paddr = 0xff;
    *pit_pbddr = 0xff;

    jumpers = (~(*duart_ip)) & 0x3f;

    is_xr = (jumpers & 0x04)?YES:NO;

    setup_duart(is_xr, is_xr);

    set_isr_handler(32, (unsigned int)trap0_handler);
    set_isr_handler(46, (unsigned int)trap14_handler);

    kprintf("\n\n\nMega-Micros SBC-3 Computer System\n");
    kprintf("MicroMon ROM V%d.%d_%03d starting.\n", major, minor, MAGIC_BUILD_NUMBER);
    kprintf("\nHardware:\n\n");

    padstr("RAM detected", PAD_COL); 
    kprintf(" [ $%06x-$%06x ]\n", 0, ram_end);

    if (jumpers & 0x04) {
        padstr("xr68C681 Extended duart detected", PAD_COL);
    }
    else {
        padstr("mc68681 generic duart detected", PAD_COL);
    }
    kprintf(" [ $%06x-$%06x ]\n", duart_base, duart_base + 30);

    // print Data about the code sections
    kprintf("\nFirmware:\n");
    pr_section("init  ", (uint32_t)&_pretext_start, (uint32_t)&_postinit_end);
    pr_section("code  ", (uint32_t)&_code_start,    (uint32_t)&_code_end);
    pr_section("rodata", (uint32_t)&_rodata_start,  (uint32_t)&_rodata_end);
    pr_section("bss   ", (uint32_t)&_bss_start,     (uint32_t)&_bss_end);
    kprintf   ("data   0x%06x   0x%06x               <-- relocated from 0x%08x\n", 
        (uint32_t)&_d_start, (uint32_t)&_d_start + (uint32_t)&_data_length, (uint32_t)&_data_load_start);

    kprintf("\nJumpers JB2: ");
    for (int i=0; i<6; i++) {
        if (jumpers & (1<<i)) {
            if (list_started == YES) {
                kprintf(", ");
            }
            kprintf("%s", jumper_txt[i]);
            list_started = YES;
        }
    }
    kprintf("\n\n");
}

bool_t is_command(const char *cmd, const char *target, int min_target_len) {
    int cmd_len = strlen(cmd);
    int target_len = strlen(target);

    if (min_target_len == 0) {
        min_target_len = target_len;
    }

    if ((cmd_len > target_len) || (cmd_len < min_target_len)) {
        return NO;
    }

    for (int i=0; i<cmd_len; i++) {
        if (tolower(cmd[i]) != tolower(target[i])) {
            return NO;
        }
    }

    return YES;
}

bool_t is_blank(const char *str) {
    while (*str) {
        if (!isblank(*str++)) {
            return NO;
        }
    }

    return YES;
}

void print_help(int argc, char *argv[]) {
    argc--;
    argv++;

    kprintf("Commands are:\n");
    kprintf("  dump [<start_address> [<count>]]\n");
    kprintf("  eval <expression>\n");
    kprintf("  go <address>]\n");
    kprintf("  load\n");
    kprintf("  probe\n");
    kprintf("  rtc (erase) | (time [hh:mm[:ss]]) | (date [yyyy:mm:dd])\n");
    kprintf("  usb1|2 baud <baudrate>\n");
    kprintf("  quit\n\n");
}

void handle_zob_command(int argc, char *argv[]) {
    register const char *cmd = argv[1];

    if ((argc == 1) || ((argc == 2) && is_command(cmd, "help", 2) == YES)) {
        kprintf("zob accepts the following sub-commands: \n", argv[0]);
        kprintf("  duart             read bytes from the duart\n");
        kprintf("\n");
    }
    else if (is_command(cmd, "duart", 2) == YES) {
        long val;
        expr_error_t res;
        int error_pos;
        int num_read = 0;
        uint32_t end_time = ticks() + (60*1000);

        res = expr_evaluate(argv[2], &val, &error_pos);
        if (res != EXPR_OK) {
            kprintf("Couldn't evaluate expression: '%s'\n", argv[2]);
            return;
        }

        kprintf("Ok, I'm waiting to read %d characters. Please start typing...\n", val);
        while ((ticks() < end_time) && (num_read < val)) {
            num_read = kio_rx_info();
            bios_printf(0, "num_read=%d\r", num_read);
        }
        bios_printf(0, "\n");
        
    }
    else {
        kprintf("'%s' is not a valid sub-command.\n", argv[1]);
    }

}

void handle_command(int argc, char *argv[]) {
    register const char *cmd = argv[0];

    argc += 0;

    if (is_blank(cmd) == YES) {
        return;
    }

    for (unsigned int i=0; i<NUM_COMMANDS; i++) {
        if (is_command(cmd, commands[i].command, commands[i].min_required) == YES) {
            // call the handler
            commands[i].handler(argc, argv);
            return;
        }
    }

    kprintf("'%s': Unrecognised command.\n", argv[0]);
}

void main(void) {
    char cmd_line[MAX_LINE];

    setup();

    while (1) {
        kprintf("# ");

        if (kgets(cmd_line) != NULL) {
            char *argv[MAX_ARGS];
            int argc = split_str(cmd_line, ' ', argv, MAX_ARGS);

            if (argc) {
                handle_command(argc, argv);
            }
        }
    }
}