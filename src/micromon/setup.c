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
#include <unistd.h>
#include <machine.h>

#include "micromon.h"

#define PAD_COL 45

static int major = 0;
static int minor = 4;
static int MAGIC_BUILD_NUMBER = 426;


uint32_t ram_end;
bool_t duart_present;
bool_t pit_present;
bool_t cf_present;
bool_t hex_display_present;
bool_t acrtc3_present;
bool_t rtc_present;
bool_t oled_present;
bool_t experimental;

static char *jumper_txt[6] = {"TxA_EN", "TxB_EN", "XR_EN", "EMU_Boot", "Experimental", "ACRTC_MODE"};

static void pr_section(const char *name, void *start, void *end) {
    printk("%-8s $%06X-$%06X   %d\n", name, start, end, (uint32_t)end - (uint32_t)start);
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

static void pr_info(const char *msg, uint32_t start, uint32_t end) {
    padstr(msg, PAD_COL);
    printk(" [ $%06X-$%06X ]\n", start, end);
}

static void pr_i2c(const char *msg, uint8_t addr) {
    uint8_t mask = 0x40;

    padstr(msg, PAD_COL);
    printk(" [ DEV ID:");

    while (mask) {
        if (addr & mask) {
            kputchar('1');
        }
        else {
            kputchar('0');
        }
        mask = mask >> 1;
    }
    printk("x ]\n");
}

static void pr_jumpers(uint8_t jumpers) {
    int list_started = NO;

    printk("\nJumpers JB2: ");
    for (int i=0; i<6; i++) {
        if (jumpers & (1<<i)) {
            if (list_started == YES) {
                printk(", ");
            }
            printk("%s", jumper_txt[i]);
            list_started = YES;
        }
    }
    printk("\n\n");

}


void setup(void) {
    uint8_t jumpers;
    int is_xr;
    char tmp_str[64];

    ram_end = get_ram_end();
    pit_present = is_pit_present();
    duart_present = is_duart_present();
    cf_present = YES;
    hex_display_present = NO;
    acrtc3_present = NO;
    rtc_present = is_rtc_present();
    oled_present = is_oled_present();

    if (pit_present == YES) {
        *pit_tivr = PIT_VECTOR_NUMBER;
        _claim_pit();

        // Set PIT ports A and B as outputs so we can drive the
        // LEDs (if present).
        *pit_paddr = 0xff;
        *pit_pbddr = 0xff;
    }

    jumpers = (~(*duart_ip)) & 0x3f;
    
    is_xr = (jumpers & 0x04)?YES:NO;
    experimental = (jumpers & 0x10)?YES:NO;

    // constants used by the expression evaluator
    init_constants();

    setup_duart(is_xr);

    set_isr_handler(32, (unsigned int)trap0_handler);
    set_isr_handler(46, (unsigned int)trap14_handler);

    printk("\n\n\nMega-Micros SBC-3 Computer System\n");
    printk("MicroMon System ROM V%d.%d_%03d starting.\n", major, minor, MAGIC_BUILD_NUMBER);
    printk("\nHardware:\n\n");

    // RAM
    snprintf(tmp_str, sizeof(tmp_str), "%dMB RAM detected", (ram_end+1)/(1024*1024));
    pr_info(tmp_str, 0, ram_end);

    // Duart
    snprintf(tmp_str, sizeof(tmp_str), "%s duart running at %sMHz", 
            (jumpers & 0x04)?"xr68c681":"generic mc68681",
            duart_clock_doubled()?"7.3728":"3.6864"
        );
    pr_info(tmp_str, (uint32_t)duart_base, (uint32_t)duart_opr_reset);

    // PI/T
    if (pit_present) {
        pr_info("68230 PI/T detected", (uint32_t)pit_base, (uint32_t)pit_tsr);
    }

    // CF
    if (cf_present) {
        pr_info("Compact Flash hardware detected", (uint32_t)cf_base, (uint32_t)cf_reg_command);
    }

    // Hex Display
    if (hex_display_present) {
        pr_info("Hex Display board detected", 0xab0000, 0xab0003);
    }

    // ACRTC3
    if (acrtc3_present) {
        pr_info("ACRTC3 detected", 0xaa0000, 0xaaffff);
    }

    if (rtc_present) {
        pr_i2c("I2C RTC detected", DS1307_ADDR);
    }

    if (oled_present) {
        pr_i2c("I2C oled display detected", SH1107_ADDR);
        if (sh1107_init() < 0) {
            printk(" --> not functioning correctly - disabling\n");
            oled_present = NO;
        }
    }

    // Jumpers
    pr_jumpers(jumpers);

    // print Data about the code sections
    printk("\nFirmware:\n");
    pr_section("init",    &_pretext_start, &_postinit_end);
    pr_section("code",    &_code_start,    &_code_end);
    pr_section("ro data", &_rodata_start,  &_rodata_end);
    printk   ("%-8s $%06X-$%06X               <-- relocated from $%08X\n", "rw data", (uint32_t)&_d_start, (uint32_t)&_d_start + (uint32_t)&_data_length, (uint32_t)&_data_load_start);
    pr_section("bss",     &_bss_start,     &_bss_end);

    if (rtc_present) {
        ds1307_time_t t;
        int status;

        i2c_speed(100);
        status = ds1307_read_time(&t);
        if (status >= 0) {
            status = ds1307_read_time(&t);

            printk("\nThe current time is: %02u:%02u:%02u on %02u-%02u-%04u %s\n", 
                t.hours, t.minutes, t.seconds,
                t.date, t.month, t.year,
                (status == 1)?"(halted)":""            );
        }
    }

    if (oled_present) {
        i2c_speed(400);
        sh1107_clear();
        sh1107_pstr(0, 0, "SBC-3\n\nusb1@230400\nusb2@230400", NULL);
        sh1107_display();
    }

    // Setup the heap so malloc can be used
    _init_heap();
    heap_print_free();

    // Activate any block devices
    printk("\nActivating block devices\n");
    bd_init();
    
    // Prepare filesystems for use
    if (experimental) {
        char pwd[PATH_MAX];
        
        printk("Attempting mounts\n");
        vfs_init();

        kprintf("PWD is '%s'\n", getcwd(pwd, PATH_MAX));
    }

    kprintf("Entering command loop.\n");
}
