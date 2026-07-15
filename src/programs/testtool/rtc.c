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
#include <nonstd.h>

#include "testtool.h"

static const char *dayNames[8] = {"?", "Sunday", "Monday", "Tuesday", "Wednesday","Thursday", "Friday", "Saturday"};

void handle_rtc_command(int argc, char *argv[]) {
    register const char *cmd = argv[1];

    if ((argc == 1) || ((argc == 2) && is_command(cmd, "help", 2) == YES)) {
        printf("rtc accepts the following sub-commands: \n");
        printf("  erase             erase the contents of the rtc's nvram\n");
        printf("  time hh:mm[:ss]   set the time in the rtc\n");
        printf("\n");
    }
    else if (is_command(cmd, "time", 2) == YES) {
        printf("Bob hasn't coded that, tey!!!\n");
    }
    else if (is_command(cmd, "erase", 2) == YES) {
        uint8_t ram[56];
        int res;

        memset(ram, 0xff, 56);
        ram[0] = 0xb0;
        ram[1] = 0xba;

        printf("Erasing rtc nvram...");

        res = ds1307_write_nvram(0, ram, 56);

        printf("\n");

        if (res != 56) {
            printf("ERROR! Erase failed.\n");
        } 
        else {
            printf("Success.\n");
        }
    }
    else {
        printf("'%s' is not a valid sub-command.\n", argv[1]);
    }
}

void handle_rtc_subcommand(int argc, char *argv[]) {
    ds1307_time_t t;
    int status;
 
    if ((argc < 1) || (argc > 2)) {
        printf("usage: show rtc [nvram]\n");
        return;
    }

    if ((argc > 1) && (is_command(argv[1], "help", 2) == YES)) {
        printf("show rtc accepts the following arguments and sub-commands:\n");
        printf("  <no arg>              read and print the time.\n");
        printf("  nvram                 dump the contents of the nvram (56 bytes)\n");
    }
    else if (argc == 1) {
        // Display the date and time
        status = ds1307_read_time(&t);
        if (status < 0) {
            printf("DS1307: no ACK from device -- check address/wiring/pull-ups\n");
        }
    
        printf("DS1307 date and time: %s 20%02u-%02u-%02u %02u:%02u:%02u\n",
            dayNames[t.day], t.year, t.month, t.date,
            t.hours, t.minutes, t.seconds);
    
        if (status == 1)
            printf("Warning: clock-halt (CH) bit is set -- oscillator is "
                "stopped, time above is not advancing until the clock "
                "is (re)started.\n");
    }
    else if (is_command(argv[1], "nvram", 2) == YES) {
        uint8_t nvram[56];

        if (ds1307_read_nvram(0, nvram, 56) != 56) {
            printf("Failed to read RTC nvram!\n");
            return;
        }

        printf("RTC nvram:\n");
        dump_mem(nvram, 56, YES);
    }
 
}

