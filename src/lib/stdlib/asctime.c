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

static char time_str[256];

static char *day_names[7] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
static char *month_names[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", 
                                 "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

char *asctime_r(const struct tm *tm, char *buf) {
    snprintf(buf, sizeof(time_str), "%s %s %d %02d:%02d:%02d %04d (GMT)\n", 
        day_names[tm->tm_wday], month_names[tm->tm_mon], tm->tm_mday,
        tm->tm_hour, tm->tm_min, tm->tm_sec, 1900+tm->tm_year);

    return buf;
}

char *asctime(const struct tm *tm) {
    return asctime_r(tm, time_str);
}

