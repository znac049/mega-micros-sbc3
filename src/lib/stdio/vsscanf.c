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
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>

/*
 * C99 vsscanf().
 *
 * Conversions: d i o u x X p n c s [ e f g E G a A F and %%, with the
 * assignment-suppressing *, a maximum field width and the length
 * modifiers hh h l ll j z t. (L, for long double, is not supported: a
 * %L floating conversion fails to match.)
 *
 * Returns the number of fields assigned, or EOF if the input ran out
 * before the first conversion.
 */

// Length modifiers
enum { LEN_INT, LEN_HH, LEN_H, LEN_L, LEN_LL, LEN_J, LEN_Z, LEN_T, LEN_BIG_L };

// The longest number we'll look at, whatever the field width
#define NUM_BUF 80

static void store_signed(void *p, int len, long long v) {
    switch (len) {
        case LEN_HH: *(signed char *)p = (signed char)v; break;
        case LEN_H:  *(short *)p = (short)v; break;
        case LEN_L:  *(long *)p = (long)v; break;
        case LEN_LL:
        case LEN_J:  *(long long *)p = v; break;
        case LEN_Z:  *(size_t *)p = (size_t)v; break;
        case LEN_T:  *(ptrdiff_t *)p = (ptrdiff_t)v; break;
        default:     *(int *)p = (int)v; break;
    }
}

static void store_unsigned(void *p, int len, unsigned long long v) {
    switch (len) {
        case LEN_HH: *(unsigned char *)p = (unsigned char)v; break;
        case LEN_H:  *(unsigned short *)p = (unsigned short)v; break;
        case LEN_L:  *(unsigned long *)p = (unsigned long)v; break;
        case LEN_LL:
        case LEN_J:  *(unsigned long long *)p = v; break;
        case LEN_Z:  *(size_t *)p = (size_t)v; break;
        case LEN_T:  *(ptrdiff_t *)p = (ptrdiff_t)v; break;
        default:     *(unsigned int *)p = (unsigned int)v; break;
    }
}

/*
 * Parse a scanset, starting just after its '['. Fills in which of the
 * 256 characters it accepts and returns a pointer past its ']', or NULL
 * if it isn't closed.
 */
static const char *parse_scanset(const char *f, unsigned char set[256]) {
    int negate = 0;
    int i;

    memset(set, 0, 256);

    if (*f == '^') {
        negate = 1;
        f++;
    }

    // A ']' straight after the '[' (or '[^') is a member
    if (*f == ']') {
        set[']'] = 1;
        f++;
    }

    while (*f != ']') {
        unsigned char lo;

        if (*f == '\0') {
            return NULL;
        }

        lo = (unsigned char)*f++;
        if ((*f == '-') && (f[1] != ']') && (f[1] != '\0') && ((unsigned char)f[1] >= lo)) {
            unsigned char hi = (unsigned char)f[1];

            for (i = lo; i <= hi; i++) {
                set[i] = 1;
            }
            f += 2;
        }
        else {
            set[lo] = 1;
        }
    }

    if (negate) {
        for (i = 0; i < 256; i++) {
            set[i] = !set[i];
        }
    }

    set[0] = 0;     // the terminator never matches
    return f + 1;
}

int vsscanf(const char *str, const char *format, va_list ap) {
    const unsigned char *s = (const unsigned char *)str;
    const char *f = format;
    int assigned = 0;
    int conversions = 0;        // conversions done, assigned or not

    while (*f != '\0') {
        int suppress = 0;
        int width = 0;
        int len = LEN_INT;
        char conv;

        // White space in the format matches any amount, including none
        if (isspace((unsigned char)*f)) {
            while (isspace((unsigned char)*f)) {
                f++;
            }
            while (isspace(*s)) {
                s++;
            }
            continue;
        }

        // An ordinary character must match exactly
        if (*f != '%') {
            if (*s == '\0') {
                goto input_failure;
            }
            if (*s != (unsigned char)*f) {
                return assigned;
            }
            s++;
            f++;
            continue;
        }

        f++;
        if (*f == '*') {
            suppress = 1;
            f++;
        }

        while (isdigit((unsigned char)*f)) {
            width = width * 10 + (*f++ - '0');
        }

        switch (*f) {
            case 'h':
                f++;
                len = LEN_H;
                if (*f == 'h') {
                    f++;
                    len = LEN_HH;
                }
                break;
            case 'l':
                f++;
                len = LEN_L;
                if (*f == 'l') {
                    f++;
                    len = LEN_LL;
                }
                break;
            case 'j': f++; len = LEN_J; break;
            case 'z': f++; len = LEN_Z; break;
            case 't': f++; len = LEN_T; break;
            case 'L': f++; len = LEN_BIG_L; break;
            default:  break;
        }

        conv = *f++;
        if (conv == '\0') {
            return assigned;
        }

        // Everything but %c, %[ and %n skips leading white space
        if ((conv != 'c') && (conv != '[') && (conv != 'n')) {
            while (isspace(*s)) {
                s++;
            }
        }

        if (conv == 'n') {
            if (!suppress) {
                store_signed(va_arg(ap, void *), len, (long long)((const char *)s - str));
            }
            continue;
        }

        if (*s == '\0') {
            goto input_failure;
        }

        switch (conv) {
            case '%':
                if (*s != '%') {
                    return assigned;
                }
                s++;
                continue;

            case 'd': case 'i': case 'o': case 'u': case 'x': case 'X': case 'p': {
                char buf[NUM_BUF];
                char *end;
                int base;
                int n = 0;
                int max = ((width > 0) && (width < NUM_BUF)) ? width : (NUM_BUF - 1);

                switch (conv) {
                    case 'd': base = 10; break;
                    case 'i': base = 0;  break;
                    case 'o': base = 8;  break;
                    case 'u': base = 10; break;
                    default:  base = 16; break;
                }

                while ((n < max) && (s[n] != '\0') && !isspace(s[n])) {
                    buf[n] = (char)s[n];
                    n++;
                }
                buf[n] = '\0';

                // strtol() takes $ as a hex prefix, but scanf doesn't
                if ((buf[0] == '$') || (((buf[0] == '+') || (buf[0] == '-')) && (buf[1] == '$'))) {
                    return assigned;
                }

                /*
                 * Out of range values do what glibc does: they saturate
                 * at the limits of long (or long long, for ll and j),
                 * and are then cut down to the size being stored.
                 */
                if ((conv == 'd') || (conv == 'i')) {
                    long long v;

                    if ((len == LEN_LL) || (len == LEN_J)) {
                        v = strtoll(buf, &end, base);
                    }
                    else {
                        v = strtol(buf, &end, base);
                    }

                    if (end == buf) {
                        return assigned;
                    }
                    if (!suppress) {
                        store_signed(va_arg(ap, void *), len, v);
                    }
                }
                else {
                    unsigned long long v;

                    if ((len == LEN_LL) || (len == LEN_J)) {
                        v = strtoull(buf, &end, base);
                    }
                    else {
                        v = strtoul(buf, &end, base);
                    }

                    if (end == buf) {
                        return assigned;
                    }
                    if (!suppress) {
                        if (conv == 'p') {
                            *va_arg(ap, void **) = (void *)(uintptr_t)v;
                        }
                        else {
                            store_unsigned(va_arg(ap, void *), len, v);
                        }
                    }
                }

                /*
                 * A 0x prefix with no hex digits after it reads as 0, and
                 * the x is taken as part of the field (as glibc does)
                 */
                if (((base == 16) || (base == 0)) && (end[0] == 'x' || end[0] == 'X') &&
                    (end > buf) && (end[-1] == '0') &&
                    ((end - 1 == buf) || (end - 2 == buf && (buf[0] == '+' || buf[0] == '-')))) {
                    end++;
                }

                s += end - buf;
                break;
            }

            case 'e': case 'f': case 'g': case 'E': case 'G': case 'a': case 'A': case 'F': {
                char buf[NUM_BUF];
                char *end;
                double v;
                int n = 0;
                int max = ((width > 0) && (width < NUM_BUF)) ? width : (NUM_BUF - 1);

                if (len == LEN_BIG_L) {
                    return assigned;
                }

                while ((n < max) && (s[n] != '\0') && !isspace(s[n])) {
                    buf[n] = (char)s[n];
                    n++;
                }
                buf[n] = '\0';

                v = strtod(buf, &end);
                if (end == buf) {
                    return assigned;
                }
                if (!suppress) {
                    if (len == LEN_L) {
                        *va_arg(ap, double *) = v;
                    }
                    else {
                        *va_arg(ap, float *) = (float)v;
                    }
                }

                s += end - buf;
                break;
            }

            case 'c': {
                char *out = suppress ? NULL : va_arg(ap, char *);
                int n = 0;

                if (width == 0) {
                    width = 1;
                }

                while ((n < width) && (*s != '\0')) {
                    if (out != NULL) {
                        *out++ = (char)*s;
                    }
                    s++;
                    n++;
                }

                // Running out part way through still counts as a
                // conversion (as glibc has it); only none at all fails
                break;
            }

            case 's': {
                char *out = suppress ? NULL : va_arg(ap, char *);
                int n = 0;

                while ((*s != '\0') && !isspace(*s) && ((width == 0) || (n < width))) {
                    if (out != NULL) {
                        *out++ = (char)*s;
                    }
                    s++;
                    n++;
                }

                if (out != NULL) {
                    *out = '\0';
                }
                break;
            }

            case '[': {
                unsigned char set[256];
                char *out;
                int n = 0;

                f = parse_scanset(f, set);
                if (f == NULL) {
                    return assigned;
                }

                out = suppress ? NULL : va_arg(ap, char *);
                while (set[*s] && ((width == 0) || (n < width))) {
                    if (out != NULL) {
                        *out++ = (char)*s;
                    }
                    s++;
                    n++;
                }

                if (n == 0) {
                    return assigned;
                }
                if (out != NULL) {
                    *out = '\0';
                }
                break;
            }

            default:
                // An unknown conversion
                return assigned;
        }

        conversions++;
        if (!suppress) {
            assigned++;
        }
    }

    return assigned;

input_failure:
    return (conversions == 0) ? EOF : assigned;
}
