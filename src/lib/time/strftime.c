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

#include <time.h>
#include <stddef.h>
#include <string.h>

/*
 * C99 strftime(), for the "C" locale. The monitor keeps UTC, and
 * localtime() is gmtime(), so %z is +0000 and %Z is UTC.
 */

static const char *const day_names[7] = {
    "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"
};

static const char *const month_names[12] = {
    "January", "February", "March", "April", "May", "June", "July",
    "August", "September", "October", "November", "December"
};

typedef struct {
    char *buf;
    size_t size;        // room in buf, including the terminator
    size_t len;         // characters stored so far
    int overflow;
} out_t;

static void put_char(out_t *o, char c) {
    if (o->len + 1 < o->size) {
        o->buf[o->len++] = c;
    }
    else {
        o->overflow = 1;
    }
}

static void put_str(out_t *o, const char *s) {
    while (*s != '\0') {
        put_char(o, *s++);
    }
}

// A number, at least width digits, padded with pad ('0' or ' ')
static void put_num(out_t *o, long n, int width, char pad) {
    char digits[12];
    int i = 0;
    int neg = (n < 0);
    unsigned long u = neg ? -(unsigned long)n : (unsigned long)n;

    do {
        digits[i++] = (char)('0' + (u % 10));
        u /= 10;
    } while (u != 0);

    if (neg) {
        put_char(o, '-');
        width--;
    }
    for (; width > i; width--) {
        put_char(o, pad);
    }
    while (i > 0) {
        put_char(o, digits[--i]);
    }
}

static int is_leap(long year) {
    return ((year % 4) == 0) && (((year % 100) != 0) || ((year % 400) == 0));
}

// Week number of the year, where weeks start on first_day (0 Sunday, 1 Monday)
static int week_number(const struct tm *tm, int first_day) {
    int wday = (tm->tm_wday + 7 - first_day) % 7;

    return (tm->tm_yday + 7 - wday) / 7;
}

/*
 * ISO 8601 week-based year and week: weeks start on Monday, and week 1
 * is the one containing the year's first Thursday.
 */
static void iso_week(const struct tm *tm, long *year_out, int *week_out) {
    long year = tm->tm_year + 1900L;
    int wday = (tm->tm_wday + 6) % 7;               // Monday 0 .. Sunday 6
    int week = (tm->tm_yday - wday + 10) / 7;

    if (week < 1) {
        // One of the last weeks of the previous year
        int prev_days = is_leap(year - 1) ? 366 : 365;

        year--;
        week = (tm->tm_yday + prev_days - wday + 10) / 7;
    }
    else {
        int days = is_leap(year) ? 366 : 365;

        // Week 1 of next year, if this week's Thursday falls there
        if (tm->tm_yday - wday + 3 >= days) {
            year++;
            week = 1;
        }
    }

    *year_out = year;
    *week_out = week;
}

static void format(out_t *o, const char *fmt, const struct tm *tm) {
    for (; *fmt != '\0'; fmt++) {
        long year;
        int week;

        if (*fmt != '%') {
            put_char(o, *fmt);
            continue;
        }

        fmt++;
        // The E and O modifiers select alternative forms, which the
        // "C" locale doesn't have
        if ((*fmt == 'E') || (*fmt == 'O')) {
            fmt++;
        }

        switch (*fmt) {
            case 'a':
                if ((unsigned)tm->tm_wday < 7) {
                    char s[4];

                    memcpy(s, day_names[tm->tm_wday], 3);
                    s[3] = '\0';
                    put_str(o, s);
                }
                else {
                    put_char(o, '?');
                }
                break;

            case 'A':
                put_str(o, ((unsigned)tm->tm_wday < 7) ? day_names[tm->tm_wday] : "?");
                break;

            case 'b':
            case 'h':
                if ((unsigned)tm->tm_mon < 12) {
                    char s[4];

                    memcpy(s, month_names[tm->tm_mon], 3);
                    s[3] = '\0';
                    put_str(o, s);
                }
                else {
                    put_char(o, '?');
                }
                break;

            case 'B':
                put_str(o, ((unsigned)tm->tm_mon < 12) ? month_names[tm->tm_mon] : "?");
                break;

            case 'c':
                format(o, "%a %b %e %H:%M:%S %Y", tm);
                break;

            case 'C':
                put_num(o, (tm->tm_year + 1900L) / 100, 2, '0');
                break;

            case 'd':
                put_num(o, tm->tm_mday, 2, '0');
                break;

            case 'D':
                format(o, "%m/%d/%y", tm);
                break;

            case 'e':
                put_num(o, tm->tm_mday, 2, ' ');
                break;

            case 'F':
                format(o, "%Y-%m-%d", tm);
                break;

            case 'g':
                iso_week(tm, &year, &week);
                put_num(o, ((year % 100) + 100) % 100, 2, '0');
                break;

            case 'G':
                iso_week(tm, &year, &week);
                put_num(o, year, 1, '0');
                break;

            case 'H':
                put_num(o, tm->tm_hour, 2, '0');
                break;

            case 'I':
                put_num(o, ((tm->tm_hour + 11) % 12) + 1, 2, '0');
                break;

            case 'j':
                put_num(o, tm->tm_yday + 1, 3, '0');
                break;

            case 'm':
                put_num(o, tm->tm_mon + 1, 2, '0');
                break;

            case 'M':
                put_num(o, tm->tm_min, 2, '0');
                break;

            case 'n':
                put_char(o, '\n');
                break;

            case 'p':
                put_str(o, (tm->tm_hour < 12) ? "AM" : "PM");
                break;

            case 'r':
                format(o, "%I:%M:%S %p", tm);
                break;

            case 'R':
                format(o, "%H:%M", tm);
                break;

            case 'S':
                put_num(o, tm->tm_sec, 2, '0');
                break;

            case 't':
                put_char(o, '\t');
                break;

            case 'T':
                format(o, "%H:%M:%S", tm);
                break;

            case 'u':
                put_num(o, (tm->tm_wday == 0) ? 7 : tm->tm_wday, 1, '0');
                break;

            case 'U':
                put_num(o, week_number(tm, 0), 2, '0');
                break;

            case 'V':
                iso_week(tm, &year, &week);
                put_num(o, week, 2, '0');
                break;

            case 'w':
                put_num(o, tm->tm_wday, 1, '0');
                break;

            case 'W':
                put_num(o, week_number(tm, 1), 2, '0');
                break;

            case 'x':
                format(o, "%m/%d/%y", tm);
                break;

            case 'X':
                format(o, "%H:%M:%S", tm);
                break;

            case 'y':
                put_num(o, (((tm->tm_year + 1900L) % 100) + 100) % 100, 2, '0');
                break;

            case 'Y':
                put_num(o, tm->tm_year + 1900L, 1, '0');
                break;

            case 'z':
                put_str(o, "+0000");
                break;

            case 'Z':
                put_str(o, "UTC");
                break;

            case '%':
                put_char(o, '%');
                break;

            case '\0':
                // A lone '%' at the end
                put_char(o, '%');
                return;

            default:
                // Unknown: copy it as it is
                put_char(o, '%');
                put_char(o, *fmt);
                break;
        }
    }
}

/*
 * Returns the number of characters stored, not counting the terminator,
 * or 0 if they didn't all fit (and then the buffer's contents are
 * unspecified, as in C99).
 */
size_t strftime(char *s, size_t max, const char *fmt, const struct tm *tm) {
    out_t o;

    o.buf = s;
    o.size = max;
    o.len = 0;
    o.overflow = 0;

    format(&o, fmt, tm);

    if (max > 0) {
        s[o.len] = '\0';
    }

    return o.overflow ? 0 : o.len;
}
