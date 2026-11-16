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
        printk("Failed to parse expression '%s'\n%s\n\n", exp, expr_error_string(res));
        return -1;
    }

    return val;
}

static int read_clock(ds1307_time_t *t) {
    int status = ds1307_read_time(t);

    if (status < 0) {
        printk("DS1307: no ACK from device -- check address/wiring/pull-ups\n");
        return NOT_OK;
    }

    if (status == 1) {
        printk("Warning: clock-halt (CH) bit is set -- oscillator is "
            "stopped, time above will not advance until the clock "
            "is (re)started. Time is standing still!\n");
    }
    
    return OK;
}

static long get_arg(int argc, char *argv[], int index, const char *what, long min, long max) {
    long val;

    if (index >= argc) {
        return -1;
    }

    val = do_eval(argv[index]);
    if (val == -1) {
        return -1;
    }

    if ((val < min) || (val > max)) {
        printk("The %s must be between %d and %d\n", what, min, max);
        return -1;
    }

    return val;
}

static int is_leap_year(int year) {
    return ((year % 4) == 0) && (((year % 100) != 0) || ((year % 400) == 0));
}
static int days_in_month(int year, int month) {
    static const uint8_t days[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    if ((month == 2) && is_leap_year(year)) {
        return 29;
    }

    return days[month-1];
}

// Sakamoto's method - google it!
static int day_of_week(int year, int month, int date) {
    static const uint8_t offsets[12] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};

    if (month < 3) {
        year--;
    }

    return ((year + year/4 - year/100 + year/400 + offsets[month - 1] + date) % 7) + 1;
}

static void do_date_command(int argc, char *argv[]) {
    ds1307_time_t t;
    long year;
    long month;
    long date;

    if (read_clock(&t) == NOT_OK) {
        return;
    }

    if (argc == 0) {
        printk("%s %02d-%02d-%04d\n", day_names[(t.day <= 7)?t.day:0], t.date, t.month, t.year + 2000);
        return;
    }

    if (argc != 3) {
        printk("usage: rtc date [yyyy mm dd]\n");
        return;
    }

    year = get_arg(argc, argv, 0, "year", 2000, 2099);
    month = get_arg(argc, argv, 1, "month", 1, 12);
    if ((year == -1) || (month == -1)) {
        return;
    }

    date = get_arg(argc, argv, 2, "date", 1, days_in_month(year, month));
    if (date == -1) {
        return;
    }

    t.year = (uint8_t)(year - 2000);
    t.month = (uint8_t)month;
    t.date = (uint8_t)date;
    t.day = (uint8_t)day_of_week(year, month, date);

    if (ds1307_write_time(&t) != OK) {
        printk("Failed to set the date.\n");
        return;
    }

    printk("Date set: %s %02d-%02d-%04d\n", day_names[t.day], t.date, t.month, t.year + 2000);
}

static void do_time_command(int argc, char *argv[]) {
    ds1307_time_t t;
    long hours;
    long mins;
    long secs;

    if (read_clock(&t) == NOT_OK) {
        return;
    }

    if (argc == 0) {
        printk("%02u:%02u:%02u\n", t.hours, t.minutes, t.seconds);
        return;
    }

    if ((argc < 2) || (argc > 3)) {
        printk("usage: rtc time [hh mm [ss]]\n");
        return;
    }

    hours = get_arg(argc, argv, 0, "hour", 0, 23);
    mins = get_arg(argc, argv, 1, "mins", 0, 59);
    secs = get_arg(argc, argv, 2, "secs", 0, 59);

    if ((hours == -1) || (mins == -1) || (secs == -1)) {
        return;
    }

    t.hours = (uint8_t)hours;
    t.minutes = (uint8_t)mins;
    t.seconds = (uint8_t)secs;

    if (ds1307_write_time(&t) != OK) {
        printk("Failed to set the time.\n");
        return;
    }

    printk("Time set: %02u:%02u:%02u\n", t.hours, t.minutes, t.seconds);
}

static void do_nvram_command(int argc, char *argv[]) {
    if (!argc) {
        uint8_t nvram[56];

        if (ds1307_read_nvram(0, nvram, 56) != 56) {
            printk("Failed to read RTC nvram!\n");
            return;
        }

        dump(nvram, BYTE, 56, YES, "RTC nvram", NO);
    }
    else if ((argc == 1) && is_command(argv[0], "erase", 2, ANY) == YES) {
        uint8_t ram[56];
        int res;

        memset(ram, 0xff, 56);
        ram[0] = 0xb0;
        ram[1] = 0xba;

        printk("Erasing rtc nvram...");

        res = ds1307_write_nvram(0, ram, 56);

        printk("\n");

        if (res != 56) {
            printk("ERROR! Erase failed.\n");
        } 
        else {
            printk("Success.\n");
        }
    }
}

void handle_rtc_command(int argc, char *argv[]) {
    register const char *cmd = argv[1];

    if ((argc == 1) || ((argc == 2) && is_command(cmd, "help", 2, ANY) == YES)) {
        printk("rtc accepts the following sub-commands: \n");
        printk("  date [yyyy mm dd]     show/set the date in the rtc\n");
        printk("  nvram [erase]         show/erase the contents of the rtc's nvram\n");
        printk("  time [hh mm [ss]]     show/set the time in the rtc\n");
        printk("\n");
    }
    else if (is_command(cmd, "time", 2, ANY) == YES) {
        do_time_command(argc-2, &argv[2]);
    }
    else if (is_command(cmd, "date", 2, ANY) == YES) {
        do_date_command(argc-2, &argv[2]);
    }
    else if (is_command(cmd, "nvram", 2, ANY) == YES) {
        do_nvram_command(argc-2, &argv[2]);
    }
    else {
        printk("'%s' is not a valid sub-command.\n", argv[1]);
    }
}

#if 0
void handle_rtc_subcommand(int argc, char *argv[]) {
    ds1307_time_t t;
    int status;
 
    if ((argc < 1) || (argc > 2)) {
        printk("usage: show rtc [nvram]\n");
        return;
    }

    if ((argc > 1) && (is_command(argv[1], "help", 2) == YES)) {
        printk("show rtc accepts the following arguments and sub-commands:\n");
        printk("  <no arg>              read and print the time.\n");
        printk("  nvram                 dump the contents of the nvram (56 bytes)\n");
    }
    else if (argc == 1) {
        // Display the date and time
        status = ds1307_read_time(&t);
        if (status < 0) {
            printk("DS1307: no ACK from device -- check address/wiring/pull-ups\n");
        }
    
        printk("DS1307 date and time: %s 20%02u-%02u-%02u %02u:%02u:%02u\n",
            day_names[t.day], t.year, t.month, t.date,
            t.hours, t.minutes, t.seconds);
    
        if (status == 1)
            printk("Warning: clock-halt (CH) bit is set -- oscillator is "
                "stopped, time above is not advancing until the clock "
                "is (re)started.\n");
    }
}
#endif