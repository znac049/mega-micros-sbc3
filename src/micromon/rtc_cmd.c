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

static const char *day_names[8] = {"?", "Sunday", "Monday", "Tuesday", "Wednesday","Thursday", "Friday", "Saturday"};

static long do_eval(char *exp) {
    expr_error_t res;
    long val;
    int error_pos;

    res = expr_evaluate(exp, &val, &error_pos);
    if (res != EXPR_OK) {
        kprintf("Failed to parse expression '%s'\n%s\n\n", exp, expr_error_string(res));
        return -1;
    }

    return val;
}

static void do_date_command(int argc, char *argv[]) {
    ds1307_time_t t;
    int status;

    status = ds1307_read_time(&t);
    if (status < 0) {
        kprintf("DS1307: no ACK from device -- check address/wiring/pull-ups\n");
    }
    else if (status == 1) {
        kprintf("Warning: clock-halt (CH) bit is set -- oscillator is "
            "stopped, time above will not advance until the clock "
            "is (re)started. Time is standing still!\n");
    }
    
    if (!argc) {
        // Display the date
        kprintf("%s %02d-%02d-%04d\n", day_names[t.day], t.date, t.month, t.year + 2000);    
    }
    else if ((argc >= 1) && (argc < 3)) {
        long year = do_eval(argv[0]);
        long month = 0;
        long date = 0;

        if (argc >= 1) {
            month = do_eval(argv[1]);

            if (argc >= 2) {
                date = do_eval(argv[2]);
            }
        }

        t.year = (uint8_t)year;
        t.month = (uint8_t)month;
        t.date = (uint8_t)date;

        status = ds1307_write_time(&t);
        if (status != OK) {
            kprintf("Failed to set the date.\n");
        }
        else {
            kprintf("Date set.\n");
        }
    }
}

static void do_time_command(int argc, char *argv[]) {
        ds1307_time_t t;
        int status;

        status = ds1307_read_time(&t);
        if (status < 0) {
            kprintf("DS1307: no ACK from device -- check address/wiring/pull-ups\n");
        }
        else if (status == 1) {
            kprintf("Warning: clock-halt (CH) bit is set -- oscillator is "
                "stopped, time above will not advance until the clock "
                "is (re)started. Time is standing still!\n");
        }
    
    if (!argc) {
        // Display the time
        kprintf("%02u:%02u:%02u\n", t.hours, t.minutes, t.seconds);
    
    }
    else if ((argc >= 1) && (argc < 3)) {
        long hours = do_eval(argv[0]);
        long mins = 0;
        long secs = 0;

        if (argc >= 1) {
            mins = do_eval(argv[1]);

            if (argc >= 2) {
                secs = do_eval(argv[2]);
            }
        }

        t.hours = (uint8_t)hours;
        t.minutes = (uint8_t)mins;
        t.seconds = (uint8_t)secs;

        status = ds1307_write_time(&t);
        if (status != OK) {
            kprintf("Failed to set the time.\n");
        }
        else {
            kprintf("Time set.\n");
        }
    }
}

static void do_nvram_command(int argc, char *argv[]) {
    if (!argc) {
        uint8_t nvram[56];

        if (ds1307_read_nvram(0, nvram, 56) != 56) {
            kprintf("Failed to read RTC nvram!\n");
            return;
        }

        dump(nvram, 56, YES, "RTC nvram", NO);
    }
    else if ((argc == 1) && is_command(argv[0], "erase", 2) == YES) {
        uint8_t ram[56];
        int res;

        memset(ram, 0xff, 56);
        ram[0] = 0xb0;
        ram[1] = 0xba;

        kprintf("Erasing rtc nvram...");

        res = ds1307_write_nvram(0, ram, 56);

        kprintf("\n");

        if (res != 56) {
            kprintf("ERROR! Erase failed.\n");
        } 
        else {
            kprintf("Success.\n");
        }
    }
}

void handle_rtc_command(int argc, char *argv[]) {
    register const char *cmd = argv[1];

    if ((argc == 1) || ((argc == 2) && is_command(cmd, "help", 2) == YES)) {
        kprintf("rtc accepts the following sub-commands: \n");
        kprintf("  date [yyyy mm dd]     show/set the date in the rtc\n");
        kprintf("  nvram [erase]         show/erase the contents of the rtc's nvram\n");
        kprintf("  time [hh mm [ss]]     show/set the time in the rtc\n");
        kprintf("\n");
    }
    else if (is_command(cmd, "time", 2) == YES) {
        do_time_command(argc-2, &argv[2]);
    }
    else if (is_command(cmd, "date", 2) == YES) {
        do_date_command(argc-2, &argv[2]);
    }
    else if (is_command(cmd, "nvram", 2) == YES) {
        do_nvram_command(argc-2, &argv[2]);
    }
    else {
        kprintf("'%s' is not a valid sub-command.\n", argv[1]);
    }
}

#if 0
void handle_rtc_subcommand(int argc, char *argv[]) {
    ds1307_time_t t;
    int status;
 
    if ((argc < 1) || (argc > 2)) {
        kprintf("usage: show rtc [nvram]\n");
        return;
    }

    if ((argc > 1) && (is_command(argv[1], "help", 2) == YES)) {
        kprintf("show rtc accepts the following arguments and sub-commands:\n");
        kprintf("  <no arg>              read and print the time.\n");
        kprintf("  nvram                 dump the contents of the nvram (56 bytes)\n");
    }
    else if (argc == 1) {
        // Display the date and time
        status = ds1307_read_time(&t);
        if (status < 0) {
            kprintf("DS1307: no ACK from device -- check address/wiring/pull-ups\n");
        }
    
        kprintf("DS1307 date and time: %s 20%02u-%02u-%02u %02u:%02u:%02u\n",
            day_names[t.day], t.year, t.month, t.date,
            t.hours, t.minutes, t.seconds);
    
        if (status == 1)
            kprintf("Warning: clock-halt (CH) bit is set -- oscillator is "
                "stopped, time above is not advancing until the clock "
                "is (re)started.\n");
    }
}
#endif