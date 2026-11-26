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
#include "scan.h"

/*
 * The C99 scanf() engine, shared by sscanf(), fscanf() and the rest.
 *
 * Input comes a character at a time through a scan_src_t, which can give
 * one character back. As C specifies, each conversion takes the longest
 * run of characters that starts a valid item, reading one past it and
 * giving that one back. So, as with glibc, a field that turns out not to
 * match ("0x" with no digit, "1e+") keeps what it read.
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

// The longest number we'll keep, whatever the field width
#define NUM_BUF 80

static int get(scan_src_t *src) {
    int c = src->get(src->ctx);

    if (c != EOF) {
        src->count++;
    }

    return c;
}

static void unget(scan_src_t *src, int c) {
    if (c != EOF) {
        src->unget(c, src->ctx);
        src->count--;
    }
}

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

    return f + 1;
}

static int digit_value(int c) {
    if ((c >= '0') && (c <= '9')) {
        return c - '0';
    }
    if ((c >= 'a') && (c <= 'f')) {
        return c - 'a' + 10;
    }
    if ((c >= 'A') && (c <= 'F')) {
        return c - 'A' + 10;
    }
    return 99;
}

/*
 * Read an integer field into buf: sign, 0x or 0 prefix as the base
 * allows, then digits. *base is updated for base 0. Returns how many
 * characters were taken, and whether any could be a number (*valid).
 */
static int read_integer(scan_src_t *src, char *buf, int max, int *base, int *valid) {
    int n = 0;
    int c = get(src);

    *valid = 0;

    if ((n < max) && ((c == '+') || (c == '-'))) {
        buf[n++] = (char)c;
        c = get(src);
    }

    if ((n < max) && (c == '0') && ((*base == 0) || (*base == 16))) {
        buf[n++] = (char)c;
        *valid = 1;
        c = get(src);

        if ((n < max) && ((c == 'x') || (c == 'X'))) {
            buf[n++] = (char)c;
            *base = 16;
            c = get(src);
        }
        else if (*base == 0) {
            *base = 8;
        }
    }
    else if (*base == 0) {
        *base = 10;
    }

    while ((n < max) && (digit_value(c) < *base)) {
        buf[n++] = (char)c;
        *valid = 1;
        c = get(src);
    }

    unget(src, c);
    buf[n] = '\0';

    return n;
}

// Is word (lower case) next, case-insensitively? Takes what matches.
static int read_word(scan_src_t *src, char *buf, int *n, int max, const char *word) {
    for (; *word != '\0'; word++) {
        int c = get(src);

        if ((*n >= max) || (tolower(c) != *word)) {
            unget(src, c);
            return 0;
        }

        buf[(*n)++] = (char)c;
    }

    return 1;
}

/*
 * Read a floating point field into buf: a decimal number with optional
 * fraction and exponent, or inf, infinity or nan. Returns whether it
 * starts with a number.
 */
static int read_float(scan_src_t *src, char *buf, int max) {
    int n = 0;
    int digits = 0;
    int c = get(src);

    if ((n < max) && ((c == '+') || (c == '-'))) {
        buf[n++] = (char)c;
        c = get(src);
    }

    if ((c == 'i') || (c == 'I')) {
        int ok;

        unget(src, c);
        ok = read_word(src, buf, &n, max, "inf");
        if (ok) {
            // "infinity" if it's all there; just "inf" otherwise
            int save = n;

            if (!read_word(src, buf, &n, max, "inity")) {
                n = save;
            }
        }
        buf[n] = '\0';
        return ok;
    }

    if ((c == 'n') || (c == 'N')) {
        int ok;

        unget(src, c);
        ok = read_word(src, buf, &n, max, "nan");
        buf[n] = '\0';
        return ok;
    }

    while ((n < max) && isdigit(c)) {
        buf[n++] = (char)c;
        digits++;
        c = get(src);
    }

    if ((n < max) && (c == '.')) {
        buf[n++] = (char)c;
        c = get(src);

        while ((n < max) && isdigit(c)) {
            buf[n++] = (char)c;
            digits++;
            c = get(src);
        }
    }

    if (digits == 0) {
        unget(src, c);
        buf[n] = '\0';
        return 0;
    }

    if ((n < max) && ((c == 'e') || (c == 'E'))) {
        int exp_digits = 0;

        buf[n++] = (char)c;
        c = get(src);

        if ((n < max) && ((c == '+') || (c == '-'))) {
            buf[n++] = (char)c;
            c = get(src);
        }

        while ((n < max) && isdigit(c)) {
            buf[n++] = (char)c;
            exp_digits++;
            c = get(src);
        }

        // "1e+" with no digits after: what was read is gone, but (as
        // glibc has it) the number before the 'e' still converts
        (void)exp_digits;
    }

    unget(src, c);
    buf[n] = '\0';

    return 1;
}

static void skip_space(scan_src_t *src) {
    int c;

    do {
        c = get(src);
    } while ((c != EOF) && isspace(c));

    unget(src, c);
}

// Is there more input? (Without taking any.)
static int at_end(scan_src_t *src) {
    int c = get(src);

    unget(src, c);
    return c == EOF;
}

int __scan(scan_src_t *src, const char *format, va_list ap) {
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
            skip_space(src);
            continue;
        }

        // An ordinary character must match exactly
        if (*f != '%') {
            int c = get(src);

            if (c == EOF) {
                goto input_failure;
            }
            if (c != (unsigned char)*f) {
                unget(src, c);
                return assigned;
            }
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
            skip_space(src);
        }

        if (conv == 'n') {
            if (!suppress) {
                store_signed(va_arg(ap, void *), len, src->count);
            }
            continue;
        }

        if (at_end(src)) {
            goto input_failure;
        }

        switch (conv) {
            case '%': {
                int c = get(src);

                if (c != '%') {
                    unget(src, c);
                    return assigned;
                }
                continue;
            }

            case 'd': case 'i': case 'o': case 'u': case 'x': case 'X': case 'p': {
                char buf[NUM_BUF];
                int base, valid;
                int max = ((width > 0) && (width < NUM_BUF)) ? width : (NUM_BUF - 1);

                switch (conv) {
                    case 'd': base = 10; break;
                    case 'i': base = 0;  break;
                    case 'o': base = 8;  break;
                    case 'u': base = 10; break;
                    default:  base = 16; break;
                }

                read_integer(src, buf, max, &base, &valid);
                if (!valid) {
                    return assigned;
                }

                /*
                 * Out of range values do what glibc does: they saturate
                 * at the limits of long (or long long, for ll and j),
                 * and are then cut down to the size being stored.
                 */
                if (!suppress) {
                    int wide = (len == LEN_LL) || (len == LEN_J);

                    if ((conv == 'd') || (conv == 'i')) {
                        long long v = wide ? strtoll(buf, NULL, base) : strtol(buf, NULL, base);

                        store_signed(va_arg(ap, void *), len, v);
                    }
                    else {
                        unsigned long long v = wide ? strtoull(buf, NULL, base) : strtoul(buf, NULL, base);

                        if (conv == 'p') {
                            *va_arg(ap, void **) = (void *)(uintptr_t)v;
                        }
                        else {
                            store_unsigned(va_arg(ap, void *), len, v);
                        }
                    }
                }
                break;
            }

            case 'e': case 'f': case 'g': case 'E': case 'G': case 'a': case 'A': case 'F': {
                char buf[NUM_BUF];
                int max = ((width > 0) && (width < NUM_BUF)) ? width : (NUM_BUF - 1);

                if (len == LEN_BIG_L) {
                    return assigned;
                }

                if (!read_float(src, buf, max)) {
                    return assigned;
                }

                if (!suppress) {
                    double v = strtod(buf, NULL);

                    if (len == LEN_L) {
                        *va_arg(ap, double *) = v;
                    }
                    else {
                        *va_arg(ap, float *) = (float)v;
                    }
                }
                break;
            }

            case 'c': {
                char *out = suppress ? NULL : va_arg(ap, char *);
                int n = 0;

                if (width == 0) {
                    width = 1;
                }

                // Running out part way through still counts as a
                // conversion (as glibc has it); only none at all fails
                while (n < width) {
                    int c = get(src);

                    if (c == EOF) {
                        break;
                    }
                    if (out != NULL) {
                        *out++ = (char)c;
                    }
                    n++;
                }
                break;
            }

            case 's': {
                char *out = suppress ? NULL : va_arg(ap, char *);
                int n = 0;
                int c = get(src);

                while ((c != EOF) && !isspace(c) && ((width == 0) || (n < width))) {
                    if (out != NULL) {
                        *out++ = (char)c;
                    }
                    n++;
                    c = get(src);
                }
                unget(src, c);

                if (out != NULL) {
                    *out = '\0';
                }
                break;
            }

            case '[': {
                unsigned char set[256];
                char *out;
                int n = 0;
                int c;

                f = parse_scanset(f, set);
                if (f == NULL) {
                    return assigned;
                }

                out = suppress ? NULL : va_arg(ap, char *);
                c = get(src);
                while ((c != EOF) && set[(unsigned char)c] && ((width == 0) || (n < width))) {
                    if (out != NULL) {
                        *out++ = (char)c;
                    }
                    n++;
                    c = get(src);
                }
                unget(src, c);

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
