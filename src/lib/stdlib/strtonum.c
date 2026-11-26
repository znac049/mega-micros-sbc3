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

#include <stdlib.h>
#include <ctype.h>
#include <stddef.h>

/*
 * The parsing shared by strtol(), strtoul(), strtoll() and strtoull().
 *
 * Skips white space, takes an optional sign and, for base 16 (or base 0),
 * an optional 0x prefix. Base 0 means decimal, octal after a leading 0 or
 * hex after 0x. As an extension, '$' is also taken as a hex prefix for
 * base 16 and base 0, as the monitor writes hex that way.
 *
 * Returns the magnitude, with *neg set for a minus sign and *overflow if
 * it didn't fit in 64 bits. *endptr is set past the digits, or to nptr
 * itself if there were none.
 */

static int digit_value(int c) {
    if ((c >= '0') && (c <= '9')) {
        return c - '0';
    }
    if ((c >= 'a') && (c <= 'z')) {
        return c - 'a' + 10;
    }
    if ((c >= 'A') && (c <= 'Z')) {
        return c - 'A' + 10;
    }
    return 99;
}

unsigned long long __strtonum(const char *nptr, char **endptr, int base, int *neg, int *overflow) {
    const char *s = nptr;
    unsigned long long value = 0;
    int any = 0;

    *neg = 0;
    *overflow = 0;

    if ((base < 0) || (base == 1) || (base > 36)) {
        if (endptr != NULL) {
            *endptr = (char *)nptr;
        }
        return 0;
    }

    while (isspace((unsigned char)*s)) {
        s++;
    }

    if ((*s == '+') || (*s == '-')) {
        *neg = (*s == '-');
        s++;
    }

    // A prefix only counts if a hex digit follows it: "0x" alone is a 0
    if ((base == 0) || (base == 16)) {
        if ((s[0] == '0') && ((s[1] == 'x') || (s[1] == 'X')) && (digit_value((unsigned char)s[2]) < 16)) {
            s += 2;
            base = 16;
        }
        else if ((s[0] == '$') && (digit_value((unsigned char)s[1]) < 16)) {
            s++;
            base = 16;
        }
    }

    if (base == 0) {
        base = (*s == '0') ? 8 : 10;
    }

    for (;; s++) {
        int d = digit_value((unsigned char)*s);

        if (d >= base) {
            break;
        }

        any = 1;
        if (value > (~0ULL - (unsigned)d) / (unsigned)base) {
            *overflow = 1;
        }
        else {
            value = value * (unsigned)base + (unsigned)d;
        }
    }

    if (endptr != NULL) {
        *endptr = (char *)(any ? s : nptr);
    }

    return any ? value : 0;
}
