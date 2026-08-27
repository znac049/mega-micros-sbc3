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
#include <time.h>

#define SECONDS_IN_DAY 86400
#define SECONDS_IN_HOUR 3600
#define SECONDS_IN_MINUTE 60

static struct tm gm_tm;

static int elapsed_days[2][12] = {
    {31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365},
    {31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366}
};

static inline int is_leap_year(int year) {
    return (((year % 4 == 0) && (year % 100!= 0)) || (year%400 == 0))?1:0;
}

static inline int find_month_from_day_num(int *day_num, int year) {
    register int d = *day_num;
    int month = 0;
    int table_index = is_leap_year(year);

    // printf("What month is day %d in in %d?\n", d, year);

    while (month < 12) {
        // printf(" %d < %d? month=%d\n", d, elapsed_days[table_index][month], month);

        if (d <= elapsed_days[table_index][month]) {
            *day_num = d - elapsed_days[table_index][month-1];
            return month;
        }

        month++;
    }

    // In theory, should never get here!
    return -1;
}

struct tm *gmtime_r(const time_t *timep, struct tm *result) {
    register time_t src = *timep;
    int n_days = src / SECONDS_IN_DAY;      // num days since Jan 1 1970
    int tmp = n_days;
    int year = 1970;
    int tod_secs = src % SECONDS_IN_DAY;

    result->tm_wday = (n_days + 4) % 7;

    while (tmp > 0) {
        n_days = tmp;
        tmp = tmp - (is_leap_year(year)?366:365);

        year++;
    }

    year--;

    result->tm_yday = n_days;

    result->tm_mon = find_month_from_day_num(&n_days, year);

    result->tm_mday = n_days + 1;
    result->tm_year = year - 1900;

    result->tm_hour = tod_secs / SECONDS_IN_HOUR;
    result->tm_min = (tod_secs - (result->tm_hour * SECONDS_IN_HOUR)) / 60;
    result->tm_sec = tod_secs - (result->tm_hour * SECONDS_IN_HOUR) - (result->tm_min * SECONDS_IN_MINUTE);

    result->tm_isdst = 0;

    return result;
}

struct tm *gmtime(const time_t *timep) {
    return gmtime_r(timep, &gm_tm);
}

