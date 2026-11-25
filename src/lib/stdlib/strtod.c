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
#include <string.h>
#include <errno.h>
#include <math.h>

/*
 * strtod(), correctly rounded (to nearest, ties to even), including
 * denormals. Decimal only: no hex floats.
 *
 * The decimal digits D and exponent E (value = D * 10^E) are turned into
 * big integers of 16-bit limbs, and the binary value worked out exactly:
 *   E >= 0: N = D * 10^E, then the top 64 bits of N
 *   E <  0: q = D * 2^k / 10^-E by long division, k chosen so q has
 *           about 63 bits
 * keeping note of whether anything non-zero was lost (the sticky bit),
 * then rounded to a double once. Exact, but slow for long or extreme
 * numbers (tens of milliseconds) - fine for parsing source files.
 */

#define MAX_SIG     780             // significant digits kept; the rest only count as sticky
#define LIMBS       320             // enough for 10^1100 and D shifted by 64

typedef struct {
    uint16_t l[LIMBS];
    int n;                          // limbs in use; 0 for zero
} big_t;

static big_t D, S, R;

static void big_set_small(big_t *a, uint32_t v) {
    a->n = 0;

    while (v != 0) {
        a->l[a->n++] = (uint16_t)v;
        v >>= 16;
    }
}

// a = a * k + add (k, add < 65536)
static void big_mul_add(big_t *a, uint32_t k, uint32_t add) {
    uint32_t carry = add;

    for (int i = 0; i < a->n; i++) {
        uint32_t v = (uint32_t)a->l[i] * k + carry;

        a->l[i] = (uint16_t)v;
        carry = v >> 16;
    }

    while ((carry != 0) && (a->n < LIMBS)) {
        a->l[a->n++] = (uint16_t)carry;
        carry >>= 16;
    }
}

static void big_mul_pow10(big_t *a, int e) {
    for (; e >= 4; e -= 4) {
        big_mul_add(a, 10000, 0);
    }

    for (; e > 0; e--) {
        big_mul_add(a, 10, 0);
    }
}

static int big_bits(const big_t *a) {
    int bits;
    uint16_t top;

    if (a->n == 0) {
        return 0;
    }

    top = a->l[a->n - 1];
    bits = (a->n - 1) * 16;

    while (top != 0) {
        bits++;
        top >>= 1;
    }

    return bits;
}

static int big_bit(const big_t *a, int i) {
    return (a->l[i >> 4] >> (i & 15)) & 1;
}

static void big_shl(big_t *a, int bits) {
    int words = bits >> 4, b = bits & 15;

    if (a->n == 0) {
        return;
    }

    if (b != 0) {
        uint16_t carry = 0;

        for (int i = 0; i < a->n; i++) {
            uint16_t v = a->l[i];

            a->l[i] = (uint16_t)((v << b) | carry);
            carry = (uint16_t)(v >> (16 - b));
        }

        if ((carry != 0) && (a->n < LIMBS)) {
            a->l[a->n++] = carry;
        }
    }

    if (words != 0) {
        int n = (a->n + words > LIMBS)?(LIMBS - words):a->n;

        memmove(&a->l[words], &a->l[0], n * sizeof(uint16_t));
        memset(&a->l[0], 0, words * sizeof(uint16_t));
        a->n = n + words;
    }
}

static int big_cmp(const big_t *a, const big_t *b) {
    if (a->n != b->n) {
        return (a->n > b->n)?1:-1;
    }

    for (int i = a->n - 1; i >= 0; i--) {
        if (a->l[i] != b->l[i]) {
            return (a->l[i] > b->l[i])?1:-1;
        }
    }

    return 0;
}

// a -= b (a >= b)
static void big_sub(big_t *a, const big_t *b) {
    int32_t borrow = 0;

    for (int i = 0; i < a->n; i++) {
        int32_t v = (int32_t)a->l[i] - ((i < b->n)?b->l[i]:0) - borrow;

        borrow = (v < 0);
        a->l[i] = (uint16_t)v;
    }

    while ((a->n > 0) && (a->l[a->n - 1] == 0)) {
        a->n--;
    }
}

// a = a * 2 + bit
static void big_shl1(big_t *a, int bit) {
    uint16_t carry = (uint16_t)bit;

    for (int i = 0; i < a->n; i++) {
        uint16_t v = a->l[i];

        a->l[i] = (uint16_t)((v << 1) | carry);
        carry = v >> 15;
    }

    if ((carry != 0) && (a->n < LIMBS)) {
        a->l[a->n++] = carry;
    }
}

/*
 * q * 2^bexp, plus a sticky bit for anything below q that wasn't zero,
 * rounded to the nearest double. q must be non-zero.
 */
static double make_double(uint64_t q, int bexp, int sticky, int *range) {
    union {
        double d;
        uint64_t u;
    } r;
    int top = 63, shift, ue;
    uint64_t kept;

    while (!(q & (1ULL << top))) {
        top--;
    }

    ue = top + bexp;                // the unbiased exponent

    if (ue > 1023) {
        *range = 1;
        r.u = 0x7ffULL << 52;       // infinity
        return r.d;
    }

    // Bits to drop: down to 53 significant bits, or more for a denormal
    shift = (ue >= -1022)?(top - 52):(-1074 - bexp);

    if (shift > 63) {
        *range = 1;
        return 0.0;                 // far too small
    }

    if (shift <= 0) {
        kept = q << -shift;         // exact
    }
    else {
        uint64_t rem = q & ((1ULL << shift) - 1);
        uint64_t half = 1ULL << (shift - 1);

        kept = q >> shift;

        // Round to nearest; exactly half way goes to the even one
        if ((rem > half) || ((rem == half) && (sticky || (kept & 1)))) {
            kept++;
        }
    }

    if (ue >= -1022) {
        if (kept == (1ULL << 53)) {
            kept >>= 1;             // rounding carried into a new bit
            ue++;
            if (ue > 1023) {
                *range = 1;
                r.u = 0x7ffULL << 52;
                return r.d;
            }
        }
        r.u = ((uint64_t)(ue + 1023) << 52) | (kept & ((1ULL << 52) - 1));
    }
    else {
        // Denormal: a carry into bit 52 makes it the smallest normal, as it should
        r.u = kept;
        if (kept == 0) {
            *range = 1;
        }
    }

    return r.d;
}

static int match(const char *s, const char *word) {
    while (*word) {
        if (tolower((unsigned char)*s++) != *word++) {
            return 0;
        }
    }

    return 1;
}

double strtod(const char *nptr, char **endptr) {
    const char *s = nptr;
    int negative = 0, any = 0, ndig = 0, sticky = 0, range = 0;
    long exp10 = 0;
    uint32_t chunk = 0;
    int chunk_len = 0;
    double value;

    while (isspace((unsigned char)*s)) {
        s++;
    }

    if ((*s == '+') || (*s == '-')) {
        negative = (*s++ == '-');
    }

    if (match(s, "inf")) {
        s += match(s, "infinity")?8:3;
        if (endptr != NULL) {
            *endptr = (char *)s;
        }
        return negative?-HUGE_VAL:HUGE_VAL;
    }

    if (match(s, "nan")) {
        if (endptr != NULL) {
            *endptr = (char *)(s + 3);
        }
        return negative?-__builtin_nan(""):__builtin_nan("");
    }

    D.n = 0;

    // Digits, before and after the point, four at a time into D
    for (int after = 0; ; s++) {
        if (isdigit((unsigned char)*s)) {
            int digit = *s - '0';

            any = 1;
            if ((ndig == 0) && (digit == 0)) {
                // a leading zero: only moves the point
            }
            else if (ndig < MAX_SIG) {
                chunk = chunk * 10 + digit;
                ndig++;
                if (++chunk_len == 4) {
                    big_mul_add(&D, 10000, chunk);
                    chunk = 0;
                    chunk_len = 0;
                }
            }
            else {
                sticky |= (digit != 0);
                exp10++;            // a digit we don't keep
            }

            if (after) {
                exp10--;
            }
        }
        else if ((*s == '.') && !after) {
            after = 1;
        }
        else {
            break;
        }
    }

    if (chunk_len != 0) {
        static const uint16_t p10[] = { 1, 10, 100, 1000 };

        big_mul_add(&D, p10[chunk_len], chunk);
    }

    if (!any) {
        if (endptr != NULL) {
            *endptr = (char *)nptr;
        }
        return 0.0;
    }

    if ((*s == 'e') || (*s == 'E')) {
        const char *e = s + 1;
        int eneg = 0;
        long eval = 0;

        if ((*e == '+') || (*e == '-')) {
            eneg = (*e++ == '-');
        }

        if (isdigit((unsigned char)*e)) {
            for (; isdigit((unsigned char)*e); e++) {
                if (eval < 100000) {
                    eval = eval * 10 + (*e - '0');
                }
            }
            exp10 += eneg?-eval:eval;
            s = e;
        }
    }

    if (endptr != NULL) {
        *endptr = (char *)s;
    }

    if (D.n == 0) {
        return negative?-0.0:0.0;
    }

    // Clearly out of range either way (the value is below 10^(ndig + exp10))
    if (ndig + exp10 > 310) {
        errno = ERANGE;
        return negative?-HUGE_VAL:HUGE_VAL;
    }

    if (ndig + exp10 < -330) {
        errno = ERANGE;
        return negative?-0.0:0.0;
    }

    if (exp10 >= 0) {
        int bits;
        uint64_t q = 0;

        big_mul_pow10(&D, (int)exp10);
        bits = big_bits(&D);

        // The top 64 bits, and whether any below them are set
        for (int i = 0; i < 64 && i < bits; i++) {
            q = (q << 1) | big_bit(&D, bits - 1 - i);
        }
        for (int i = 0; i < bits - 64; i++) {
            if (big_bit(&D, i)) {
                sticky = 1;
                break;
            }
        }

        value = make_double(q, (bits > 64)?(bits - 64):0, sticky, &range);
    }
    else {
        int k, abits;
        uint64_t q = 0;

        big_set_small(&S, 1);
        big_mul_pow10(&S, (int)-exp10);

        // Shift D so the quotient has 63 or 64 bits
        k = big_bits(&S) - big_bits(&D) + 63;
        if (k > 0) {
            big_shl(&D, k);
        }
        else if (k < 0) {
            big_shl(&S, -k);
        }

        // Binary long division of D by S
        R.n = 0;
        abits = big_bits(&D);
        for (int i = abits - 1; i >= 0; i--) {
            big_shl1(&R, big_bit(&D, i));
            q <<= 1;
            if (big_cmp(&R, &S) >= 0) {
                big_sub(&R, &S);
                q |= 1;
            }
        }

        sticky |= (R.n != 0);
        value = make_double(q, -k, sticky, &range);
    }

    if (range) {
        errno = ERANGE;
    }

    return negative?-value:value;
}
