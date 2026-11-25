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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * C99 vsnprintf(). Flags - + space # 0, field width and precision (either
 * may be *), lengths hh h l ll z j t L, and conversions d i u o x X c s p
 * n % f F e E g G. %p prints the pointer in hex, with no 0x unless # is
 * given, as the monitor has always done.
 *
 * Floating point output is exact: the double's full decimal value,
 * correctly rounded (ties to even), the same as glibc prints.
 *
 * Returns the length the whole output would have had: if that's size or
 * more, the output was cut short (but is always NUL-terminated if size
 * isn't 0).
 */

typedef struct {
    char *buf;
    size_t size;
    size_t len;
} out_t;

#define F_MINUS     0x01
#define F_PLUS      0x02
#define F_SPACE     0x04
#define F_HASH      0x08
#define F_ZERO      0x10

static void put(out_t *o, char c) {
    if (o->len + 1 < o->size) {
        o->buf[o->len] = c;
    }
    o->len++;
}

static void put_n(out_t *o, char c, int n) {
    while (n-- > 0) {
        put(o, c);
    }
}

static void put_s(out_t *o, const char *s, int n) {
    while (n-- > 0) {
        put(o, *s++);
    }
}

/*
 * The common tail of numbers: prefix (sign, 0x...), then zeros, then the
 * digits, padded to width on the left or right.
 */
static void emit(out_t *o, int flags, int width, const char *prefix,
                 int zeros, const char *digits, int ndigits) {
    int plen = strlen(prefix);
    int total = plen + zeros + ndigits;

    if ((flags & F_ZERO) && !(flags & F_MINUS) && (total < width)) {
        zeros += width - total;
        total = width;
    }

    if (!(flags & F_MINUS)) {
        put_n(o, ' ', width - total);
    }

    put_s(o, prefix, plen);
    put_n(o, '0', zeros);
    put_s(o, digits, ndigits);

    if (flags & F_MINUS) {
        put_n(o, ' ', width - total);
    }
}

static void format_int(out_t *o, unsigned long long v, int negative, int conv,
                       int flags, int width, int prec) {
    char digits[24];
    char prefix[3] = "";
    int n = 0;
    int base = 10;
    const char *set = "0123456789abcdef";

    if ((conv == 'x') || (conv == 'p')) {
        base = 16;
    }
    else if (conv == 'X') {
        base = 16;
        set = "0123456789ABCDEF";
    }
    else if (conv == 'o') {
        base = 8;
    }

    // Digits, least significant first
    while (v != 0) {
        digits[n++] = set[v % base];
        v /= base;
    }

    // Reverse them
    for (int i = 0; i < n / 2; i++) {
        char t = digits[i];

        digits[i] = digits[n - 1 - i];
        digits[n - 1 - i] = t;
    }

    if ((conv == 'd') || (conv == 'i')) {
        if (negative) {
            strcpy(prefix, "-");
        }
        else if (flags & F_PLUS) {
            strcpy(prefix, "+");
        }
        else if (flags & F_SPACE) {
            strcpy(prefix, " ");
        }
    }
    else if ((flags & F_HASH) && (n > 0) && (base == 16)) {
        strcpy(prefix, (conv == 'X')?"0X":"0x");
    }

    if (prec < 0) {
        prec = 1;                   // at least one digit, unless %.0d of 0
    }
    else {
        flags &= ~F_ZERO;           // a precision overrides the 0 flag
    }

    if ((conv == 'o') && (flags & F_HASH) && (prec <= n)) {
        prec = n + 1;               // # makes octal start with a 0
    }

    emit(o, flags, width, prefix, (prec > n)?(prec - n):0, digits, n);
}

#define MAX_PREC    40              // precisions above this are cut to it
#define MAX_EXACT   800             // digits in the exact value of any double

/*
 * The exact decimal value of a double, done with a simple big integer of
 * 16-bit limbs (so the 68030's own 32/32 divide does the dividing).
 * d = m * 2^e exactly, so:
 *   e >= 0: the value is the integer m * 2^e
 *   e <  0: it's m * 5^-e, with the decimal point -e places from the end
 */
#define LIMBS       160             // m * 5^1074 needs about 2550 bits

static uint16_t limb[LIMBS];
static char exact[MAX_EXACT];

static int bn_mul(int n, uint32_t k) {
    uint32_t carry = 0;

    for (int i = 0; i < n; i++) {
        uint32_t v = (uint32_t)limb[i] * k + carry;

        limb[i] = (uint16_t)v;
        carry = v >> 16;
    }

    while ((carry != 0) && (n < LIMBS)) {
        limb[n++] = (uint16_t)carry;
        carry >>= 16;
    }

    return n;
}

// Divide by 10000, returning the remainder
static uint32_t bn_div10000(int *n) {
    uint32_t rem = 0;

    for (int i = *n - 1; i >= 0; i--) {
        uint32_t v = (rem << 16) | limb[i];

        limb[i] = (uint16_t)(v / 10000);
        rem = v % 10000;
    }

    while ((*n > 0) && (limb[*n - 1] == 0)) {
        (*n)--;
    }

    return rem;
}

/*
 * All the significant digits of d (finite, > 0) into exact[] as 0-9, and
 * the decimal exponent of the first. Returns how many there are.
 */
static int exact_digits(double d, int *exp10) {
    union {
        double d;
        uint64_t u;
    } b;
    uint64_t m;
    int e, n = 0, count = 0, len, start;

    b.d = d;
    m = b.u & ((1ULL << 52) - 1);
    e = (int)((b.u >> 52) & 0x7ff);

    if (e == 0) {
        e = 1;                      // denormal: no implicit 1
    }
    else {
        m |= 1ULL << 52;
    }
    e -= 1075;                      // so d = m * 2^e

    // Drop trailing zero bits, which keeps the numbers smaller
    while (!(m & 1)) {
        m >>= 1;
        e++;
    }

    while (m != 0) {
        limb[n++] = (uint16_t)m;
        m >>= 16;
    }

    if (e >= 0) {
        for (int i = e; i > 0; i -= 15) {
            n = bn_mul(n, 1U << ((i > 15)?15:i));
        }
    }
    else {
        for (int i = -e; i > 0; i -= 6) {
            static const uint16_t pow5[] = { 1, 5, 25, 125, 625, 3125, 15625 };

            n = bn_mul(n, pow5[(i > 6)?6:i]);
        }
    }

    // Digits come out least significant first, four at a time
    while (n > 0) {
        uint32_t r = bn_div10000(&n);

        for (int k = 0; k < 4; k++) {
            exact[MAX_EXACT - 1 - count++] = (char)(r % 10);
            r /= 10;
        }
    }

    // Skip leading zeros
    start = MAX_EXACT - count;
    while ((start < MAX_EXACT - 1) && (exact[start] == 0)) {
        start++;
    }

    len = MAX_EXACT - start;
    memmove(exact, &exact[start], len);

    *exp10 = len - 1 + ((e < 0)?e:0);

    return len;
}

/*
 * The first n significant digits of d (finite, > 0), correctly rounded
 * (ties to even), as 0-9 in dig[], and the decimal exponent of the first.
 */
static void get_digits(double d, int n, char *dig, int *exp10) {
    int e, len = exact_digits(d, &e);

    for (int i = 0; i < n; i++) {
        dig[i] = (i < len)?exact[i]:0;
    }

    if (n < len) {
        int up = 0;

        if (exact[n] > 5) {
            up = 1;
        }
        else if (exact[n] == 5) {
            // Exactly half way only if nothing follows the 5
            up = (n > 0)?(dig[n - 1] & 1):0;
            for (int i = n + 1; i < len; i++) {
                if (exact[i] != 0) {
                    up = 1;
                    break;
                }
            }
        }

        if (up) {
            int i = n - 1;

            while ((i >= 0) && (++dig[i] == 10)) {
                dig[i--] = 0;
            }

            if (i < 0) {
                // 9.99... rounded up to 10
                dig[0] = 1;
                e++;
            }
        }
    }

    *exp10 = e;
}

/*
 * A long double (the 68881's 96-bit extended format) as a double, done
 * with bits: the compiler's own conversion uses FPU instructions. Only
 * needed for %Lf and friends; the result is rounded towards zero.
 */
static double long_double_to_double(long double ld) {
    union {
        long double ld;
        struct {
            uint16_t sign_exp;
            uint16_t pad;
            uint64_t mant;          // explicit leading 1
        } x;
    } in;
    union {
        double d;
        uint64_t u;
    } out;
    int exp;
    uint64_t sign;

    in.ld = ld;
    sign = (uint64_t)(in.x.sign_exp >> 15) << 63;
    exp = in.x.sign_exp & 0x7fff;

    if (exp == 0x7fff) {
        out.u = sign | (0x7ffULL << 52) | ((in.x.mant << 1) >> 12);     // inf/NaN
    }
    else if ((in.x.mant == 0) || (exp - 16383 < -1022)) {
        out.u = sign;               // zero, or too small for this conversion
    }
    else if (exp - 16383 > 1023) {
        out.u = sign | (0x7ffULL << 52);
    }
    else {
        out.u = sign | ((uint64_t)(exp - 16383 + 1023) << 52) | ((in.x.mant << 1) >> 12);
    }

    return out.d;
}

static void format_double(out_t *o, double d, int conv, int flags, int width, int prec) {
    char prefix[2] = "";
    char text[320 + MAX_PREC];          // %f of 1e308: 309 digits, ".", prec
    char dig[310 + MAX_PREC];
    int len = 0, e = 0, upper = ((conv >= 'A') && (conv <= 'Z'));
    int lconv = tolower(conv);

    if ((d < 0.0) || ((d == 0.0) && (1.0 / d < 0.0))) {
        prefix[0] = '-';
        d = -d;
    }
    else if (flags & F_PLUS) {
        prefix[0] = '+';
    }
    else if (flags & F_SPACE) {
        prefix[0] = ' ';
    }

    if ((d != d) || (d - d != 0.0)) {
        // NaN or infinity: never zero padded
        const char *s = (d != d)?(upper?"NAN":"nan"):(upper?"INF":"inf");

        emit(o, flags & ~F_ZERO, width, prefix, 0, s, 3);
        return;
    }

    if (prec < 0) {
        prec = 6;
    }

    if (prec > MAX_PREC) {
        prec = MAX_PREC;
    }

    if (lconv == 'g') {
        int p = (prec == 0)?1:prec;
        int x = 0;

        if (d != 0.0) {
            get_digits(d, p, dig, &x);
        }

        if ((x >= -4) && (x < p)) {
            lconv = 'f';
            prec = p - 1 - x;
        }
        else {
            lconv = 'e';
            prec = p - 1;
        }

        if (!(flags & F_HASH)) {
            flags |= 0x100;          // strip trailing zeros below
        }
    }

    if (lconv == 'e') {
        if (d == 0.0) {
            memset(dig, 0, prec + 1);
        }
        else {
            get_digits(d, prec + 1, dig, &e);
        }

        text[len++] = '0' + dig[0];
        if ((prec > 0) || (flags & F_HASH)) {
            text[len++] = '.';
        }
        for (int i = 1; i <= prec; i++) {
            text[len++] = '0' + dig[i];
        }

        if (flags & 0x100) {
            while ((len > 1) && (text[len - 1] == '0')) {
                len--;
            }
            if (text[len - 1] == '.') {
                len--;
            }
        }

        text[len++] = upper?'E':'e';
        text[len++] = (e < 0)?'-':'+';
        if (e < 0) {
            e = -e;
        }
        if (e >= 100) {
            text[len++] = '0' + e / 100;
        }
        text[len++] = '0' + (e / 10) % 10;
        text[len++] = '0' + e % 10;
    }
    else {
        // %f: the digits before the point, then prec after it
        int n = 0, intdigits;

        if (d != 0.0) {
            int x;

            int len = exact_digits(d, &x);  // the exponent, unrounded
            int half_up = 0;

            // If no digit is kept, does it round up to 1 in the last place?
            if (exact[0] > 5) {
                half_up = 1;
            }
            else if (exact[0] == 5) {
                for (int i = 1; i < len; i++) {
                    if (exact[i] != 0) {
                        half_up = 1;    // more than half (exactly half: to even, 0)
                        break;
                    }
                }
            }

            n = x + 1 + prec;

            if (n >= 1) {
                get_digits(d, n, dig, &e);
                if (e > x) {
                    dig[n++] = 0;       // rounding added a digit in front
                }
            }
            else if ((n == 0) && half_up) {
                // Rounds up to 1 in the last place
                e = -prec;
                dig[0] = 1;
                n = 1;
            }
            else {
                n = 0;
            }
        }

        intdigits = (n == 0)?0:(e + 1);

        if (intdigits <= 0) {
            text[len++] = '0';
        }
        else {
            for (int i = 0; i < intdigits; i++) {
                text[len++] = (i < n)?('0' + dig[i]):'0';
            }
        }

        if ((prec > 0) || (flags & F_HASH)) {
            text[len++] = '.';
        }

        for (int i = 0; i < prec; i++) {
            int k = intdigits + i;      // index into dig[] of this digit

            text[len++] = ((n > 0) && (k >= 0) && (k < n))?('0' + dig[k]):'0';
        }

        if ((flags & 0x100) && (prec > 0)) {
            while (text[len - 1] == '0') {
                len--;
            }
            if (text[len - 1] == '.') {
                len--;
            }
        }
    }

    emit(o, flags & ~0x100, width, prefix, 0, text, len);
}

int vsnprintf(char *str, size_t size, const char *format, va_list ap) {
    out_t o = { str, size, 0 };

    for (const char *f = format; *f != EOS; f++) {
        int flags = 0, width = 0, prec = -1, length = 0;
        int conv;

        if (*f != '%') {
            put(&o, *f);
            continue;
        }

        // Flags
        for (f++; ; f++) {
            if (*f == '-') flags |= F_MINUS;
            else if (*f == '+') flags |= F_PLUS;
            else if (*f == ' ') flags |= F_SPACE;
            else if (*f == '#') flags |= F_HASH;
            else if (*f == '0') flags |= F_ZERO;
            else break;
        }

        // Width
        if (*f == '*') {
            width = va_arg(ap, int);
            if (width < 0) {
                flags |= F_MINUS;
                width = -width;
            }
            f++;
        }
        else {
            while (isdigit((unsigned char)*f)) {
                width = width * 10 + (*f++ - '0');
            }
        }

        // Precision
        if (*f == '.') {
            f++;
            prec = 0;
            if (*f == '*') {
                prec = va_arg(ap, int);
                f++;
            }
            else {
                while (isdigit((unsigned char)*f)) {
                    prec = prec * 10 + (*f++ - '0');
                }
            }
        }

        // Length: 1 = char, 2 = short, 4 = (default/long), 8 = long long
        if (*f == 'h') {
            length = (f[1] == 'h')?1:2;
            f += (f[1] == 'h')?2:1;
        }
        else if (*f == 'l') {
            length = (f[1] == 'l')?8:4;
            f += (f[1] == 'l')?2:1;
        }
        else if ((*f == 'z') || (*f == 't')) {
            length = 4;
            f++;
        }
        else if (*f == 'j') {
            length = 8;
            f++;
        }
        else if (*f == 'L') {
            length = 16;            // long double
            f++;
        }

        conv = *f;
        if (conv == EOS) {
            break;
        }

        switch (conv) {
            case 'd':
            case 'i': {
                long long v = (length == 8)?va_arg(ap, long long):va_arg(ap, int);
                unsigned long long u;

                if (length == 1) {
                    v = (signed char)v;
                }
                else if (length == 2) {
                    v = (short)v;
                }

                u = (v < 0)?(0ULL - (unsigned long long)v):(unsigned long long)v;
                format_int(&o, u, v < 0, conv, flags, width, prec);
                break;
            }

            case 'u':
            case 'o':
            case 'x':
            case 'X': {
                unsigned long long v = (length == 8)?va_arg(ap, unsigned long long):va_arg(ap, unsigned int);

                if (length == 1) {
                    v = (unsigned char)v;
                }
                else if (length == 2) {
                    v = (unsigned short)v;
                }

                format_int(&o, v, 0, conv, flags, width, prec);
                break;
            }

            case 'p':
                format_int(&o, (unsigned long)va_arg(ap, void *), 0, 'p', flags, width, prec);
                break;

            case 'c': {
                char c = (char)va_arg(ap, int);

                emit(&o, flags & ~F_ZERO, width, "", 0, &c, 1);
                break;
            }

            case 's': {
                const char *s = va_arg(ap, const char *);
                int n = 0;

                if (s == NULL) {
                    s = "(null)";
                }

                while ((s[n] != EOS) && ((prec < 0) || (n < prec))) {
                    n++;
                }

                emit(&o, flags & ~F_ZERO, width, "", 0, s, n);
                break;
            }

            case 'f':
            case 'F':
            case 'e':
            case 'E':
            case 'g':
            case 'G': {
                double d = (length == 16)?long_double_to_double(va_arg(ap, long double)):va_arg(ap, double);

                format_double(&o, d, conv, flags, width, prec);
                break;
            }

            case 'n':
                *va_arg(ap, int *) = (int)o.len;
                break;

            case '%':
                put(&o, '%');
                break;

            default:
                // Unknown: print it as it was
                put(&o, '%');
                put(&o, conv);
                break;
        }
    }

    if (size > 0) {
        str[(o.len < size)?o.len:(size - 1)] = EOS;
    }

    return (int)o.len;
}
