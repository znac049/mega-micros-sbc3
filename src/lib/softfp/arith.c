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

/*
 * Software float and double arithmetic and comparisons, replacing libgcc's.
 *
 * libgcc does have soft-float versions of these (the toolchain's libgcc
 * does only the conversions with the FPU - see softfp.c), but they were
 * built as position-independent code: they reach their data through the
 * GOT, which vlink can't handle when linking a static program. With
 * these in libmega, linking a C program never needs those objects.
 *
 * Everything the libgcc objects define is here (_double.o, _float.o, and
 * the __eqdf2 ... __gesf2 wrappers), so none of them is pulled in.
 *
 * Also here, for the same reason as softfp.c: libgcc's conversions
 * between float and 64-bit integers, and __powidf2/__powisf2, use FPU
 * instructions.
 *
 * The double arithmetic follows Berkeley SoftFloat 3: significands are
 * held with the implicit 1 at bit 62 and ten bits for rounding below the
 * 53 kept, and round_pack() rounds to nearest, ties to even, handling
 * overflow and denormals. Results are the correctly rounded IEEE 754
 * ones. A NaN operand gives that NaN, quietened; an invalid operation
 * gives the 68881's default NaN (all ones).
 *
 * float arithmetic is done as double and then rounded: a double holds
 * the exact sum, difference or product of two floats to well over the
 * 2 * 24 + 2 bits needed for that second rounding to give the correctly
 * rounded float result (the same holds for the quotient).
 *
 * Nothing here may use a floating point operator: each would call one of
 * the functions being defined.
 */

#include <ctype.h>

typedef union {
    double d;
    uint64_t u;
} dbits_t;

typedef union {
    float f;
    uint32_t u;
} fbits_t;

#define DEFAULT_NAN     0x7fffffffffffffffULL
#define QUIET_BIT       0x0008000000000000ULL
#define IMPLICIT        0x0010000000000000ULL
#define MANT_MASK       0x000fffffffffffffULL

#define SIGN(u)         ((int)((u) >> 63))
#define EXP(u)          ((int)(((u) >> 52) & 0x7ff))
#define MANT(u)         ((u) & MANT_MASK)
#define IS_NAN(u)       (((u) << 1) > (0x7ffULL << 53))

double __adddf3(double a, double b);
double __subdf3(double a, double b);
double __muldf3(double a, double b);
double __divdf3(double a, double b);
double __negdf2(double a);
int __cmpdf2(double a, double b);
int __eqdf2(double a, double b);
int __nedf2(double a, double b);
int __ltdf2(double a, double b);
int __ledf2(double a, double b);
int __gtdf2(double a, double b);
int __gedf2(double a, double b);

float __addsf3(float a, float b);
float __subsf3(float a, float b);
float __mulsf3(float a, float b);
float __divsf3(float a, float b);
float __negsf2(float a);
int __cmpsf2(float a, float b);
int __eqsf2(float a, float b);
int __nesf2(float a, float b);
int __ltsf2(float a, float b);
int __lesf2(float a, float b);
int __gtsf2(float a, float b);
int __gesf2(float a, float b);

int64_t __fixsfdi(float f);
uint64_t __fixunssfdi(float f);
float __floatdisf(int64_t v);
float __floatundisf(uint64_t v);
double __powidf2(double x, int n);
float __powisf2(float x, int n);

// From softfp.c
double __extendsfdf2(float f);
float __truncdfsf2(double d);
int64_t __fixdfdi(double d);
uint64_t __fixunsdfdi(double d);
double __floatundidf(uint64_t v);

static double from_bits(uint64_t u) {
    dbits_t b;

    b.u = u;
    return b.d;
}

static uint64_t to_bits(double d) {
    dbits_t b;

    b.d = d;
    return b.u;
}

// Leading zeros of a non-zero value
static int clz64(uint64_t v) {
    uint32_t hi = (uint32_t)(v >> 32);

    return hi ? __builtin_clz(hi) : 32 + __builtin_clz((uint32_t)v);
}

// Shift right, keeping a 1 in the lowest bit if any 1s were shifted out
static uint64_t shift_right_jam(uint64_t v, int dist) {
    if (dist <= 0) {
        return v;
    }
    if (dist >= 63) {
        return v != 0;
    }

    return (v >> dist) | ((v << (64 - dist)) != 0);
}

static uint64_t pack(int sign, int exp, uint64_t sig) {
    // Added, not ORed: sig's implicit bit, if it has one, adds 1 to exp
    return ((uint64_t)sign << 63) + ((uint64_t)exp << 52) + sig;
}

/*
 * Round and pack a result. sig has its implicit 1 at bit 62 (or less, for
 * a denormal) and ten rounding bits; the value is sig / 2^62 * 2^(exp -
 * 1022), so exp is one less than the result's exponent field.
 */
static uint64_t round_pack(int sign, int exp, uint64_t sig) {
    unsigned round_bits = (unsigned)sig & 0x3ff;

    if ((unsigned)exp >= 0x7fd) {
        if (exp < 0) {
            // Denormal (or zero) result
            sig = shift_right_jam(sig, -exp);
            exp = 0;
            round_bits = (unsigned)sig & 0x3ff;
        }
        else if ((exp > 0x7fd) || (sig + 0x200 >= 0x8000000000000000ULL)) {
            return pack(sign, 0x7ff, 0);    // overflow: infinity
        }
    }

    sig = (sig + 0x200) >> 10;
    if (round_bits == 0x200) {
        sig &= ~1ULL;                       // a tie: round to even
    }
    if (sig == 0) {
        exp = 0;
    }

    return pack(sign, exp, sig);
}

// round_pack(), for a sig that may need normalising first
static uint64_t norm_round_pack(int sign, int exp, uint64_t sig) {
    int shift = clz64(sig) - 1;

    exp -= shift;
    if ((shift >= 10) && ((unsigned)exp < 0x7fd)) {
        return pack(sign, (sig != 0) ? exp : 0, sig << (shift - 10));
    }

    return round_pack(sign, exp, sig << shift);
}

// Normalise a denormal's significand (non-zero): sets *exp, returns sig
static uint64_t norm_denormal(uint64_t sig, int *exp) {
    int shift = clz64(sig) - 11;

    *exp = 1 - shift;
    return sig << shift;
}

// The result for a NaN operand: the first NaN, made quiet
static uint64_t propagate_nan(uint64_t a, uint64_t b) {
    return (IS_NAN(a) ? a : b) | QUIET_BIT;
}

// Add the magnitudes of a and b, which have the same sign
static uint64_t add_mags(uint64_t a, uint64_t b, int sign) {
    int exp_a = EXP(a), exp_b = EXP(b);
    uint64_t sig_a = MANT(a), sig_b = MANT(b);
    int diff = exp_a - exp_b;
    int exp;
    uint64_t sig;

    if (diff == 0) {
        if (exp_a == 0) {
            // Both denormal: a plain add, which may carry into normal
            return a + sig_b;
        }
        if (exp_a == 0x7ff) {
            return (sig_a | sig_b) ? propagate_nan(a, b) : a;
        }

        exp = exp_a;
        sig = (IMPLICIT * 2 + sig_a + sig_b) << 9;
    }
    else {
        sig_a <<= 9;
        sig_b <<= 9;

        if (diff < 0) {
            if (exp_b == 0x7ff) {
                return sig_b ? propagate_nan(a, b) : pack(sign, 0x7ff, 0);
            }
            exp = exp_b;
            sig_a = exp_a ? (sig_a + 0x2000000000000000ULL) : (sig_a << 1);
            sig_a = shift_right_jam(sig_a, -diff);
        }
        else {
            if (exp_a == 0x7ff) {
                return sig_a ? propagate_nan(a, b) : a;
            }
            exp = exp_a;
            sig_b = exp_b ? (sig_b + 0x2000000000000000ULL) : (sig_b << 1);
            sig_b = shift_right_jam(sig_b, diff);
        }

        sig = 0x2000000000000000ULL + sig_a + sig_b;
        if (sig < 0x4000000000000000ULL) {
            exp--;
            sig <<= 1;
        }
    }

    return round_pack(sign, exp, sig);
}

// Subtract the magnitude of b from a's, where a has the given sign
static uint64_t sub_mags(uint64_t a, uint64_t b, int sign) {
    int exp_a = EXP(a), exp_b = EXP(b);
    uint64_t sig_a = MANT(a), sig_b = MANT(b);
    int diff = exp_a - exp_b;

    if (diff == 0) {
        int64_t sig_diff;
        int shift, exp;

        if (exp_a == 0x7ff) {
            // inf - inf is invalid
            return (sig_a | sig_b) ? propagate_nan(a, b) : DEFAULT_NAN;
        }

        sig_diff = (int64_t)(sig_a - sig_b);
        if (sig_diff == 0) {
            return 0;               // x - x is +0
        }

        if (exp_a != 0) {
            exp_a--;
        }
        if (sig_diff < 0) {
            sign = !sign;
            sig_diff = -sig_diff;
        }

        shift = clz64((uint64_t)sig_diff) - 11;
        exp = exp_a - shift;
        if (exp < 0) {
            shift = exp_a;
            exp = 0;
        }

        return pack(sign, exp, (uint64_t)sig_diff << shift);
    }
    else {
        int exp;
        uint64_t sig;

        sig_a <<= 10;
        sig_b <<= 10;

        if (diff < 0) {
            sign = !sign;
            if (exp_b == 0x7ff) {
                return sig_b ? propagate_nan(a, b) : pack(sign, 0x7ff, 0);
            }
            sig_a += exp_a ? 0x4000000000000000ULL : sig_a;
            sig_a = shift_right_jam(sig_a, -diff);
            sig_b |= 0x4000000000000000ULL;
            exp = exp_b;
            sig = sig_b - sig_a;
        }
        else {
            if (exp_a == 0x7ff) {
                return sig_a ? propagate_nan(a, b) : a;
            }
            sig_b += exp_b ? 0x4000000000000000ULL : sig_b;
            sig_b = shift_right_jam(sig_b, diff);
            sig_a |= 0x4000000000000000ULL;
            exp = exp_a;
            sig = sig_a - sig_b;
        }

        return norm_round_pack(sign, exp - 1, sig);
    }
}

double __adddf3(double a, double b) {
    uint64_t ua = to_bits(a), ub = to_bits(b);

    if (SIGN(ua) == SIGN(ub)) {
        return from_bits(add_mags(ua, ub, SIGN(ua)));
    }

    return from_bits(sub_mags(ua, ub, SIGN(ua)));
}

double __subdf3(double a, double b) {
    uint64_t ua = to_bits(a), ub = to_bits(b);

    if (SIGN(ua) == SIGN(ub)) {
        return from_bits(sub_mags(ua, ub, SIGN(ua)));
    }

    return from_bits(add_mags(ua, ub, SIGN(ua)));
}

// The high 64 bits of a 128-bit product, with a sticky bit for the rest
static uint64_t mul_high_jam(uint64_t a, uint64_t b) {
    uint32_t a1 = (uint32_t)(a >> 32), a0 = (uint32_t)a;
    uint32_t b1 = (uint32_t)(b >> 32), b0 = (uint32_t)b;
    uint64_t p00 = (uint64_t)a0 * b0;
    uint64_t p01 = (uint64_t)a0 * b1;
    uint64_t p10 = (uint64_t)a1 * b0;
    uint64_t p11 = (uint64_t)a1 * b1;
    uint64_t mid = (p00 >> 32) + (uint32_t)p01 + (uint32_t)p10;
    uint64_t low = (mid << 32) | (uint32_t)p00;
    uint64_t high = p11 + (p01 >> 32) + (p10 >> 32) + (mid >> 32);

    return high | (low != 0);
}

double __muldf3(double a, double b) {
    uint64_t ua = to_bits(a), ub = to_bits(b);
    int sign = SIGN(ua) ^ SIGN(ub);
    int exp_a = EXP(ua), exp_b = EXP(ub);
    uint64_t sig_a = MANT(ua), sig_b = MANT(ub);
    uint64_t sig;
    int exp;

    if (exp_a == 0x7ff) {
        if (sig_a || ((exp_b == 0x7ff) && sig_b)) {
            return from_bits(propagate_nan(ua, ub));
        }
        // inf * 0 is invalid
        return from_bits(((exp_b | sig_b) == 0) ? DEFAULT_NAN : pack(sign, 0x7ff, 0));
    }
    if (exp_b == 0x7ff) {
        if (sig_b) {
            return from_bits(propagate_nan(ua, ub));
        }
        return from_bits(((exp_a | sig_a) == 0) ? DEFAULT_NAN : pack(sign, 0x7ff, 0));
    }

    if (exp_a == 0) {
        if (sig_a == 0) {
            return from_bits(pack(sign, 0, 0));
        }
        sig_a = norm_denormal(sig_a, &exp_a);
    }
    if (exp_b == 0) {
        if (sig_b == 0) {
            return from_bits(pack(sign, 0, 0));
        }
        sig_b = norm_denormal(sig_b, &exp_b);
    }

    exp = exp_a + exp_b - 0x3ff;
    sig_a = (sig_a | IMPLICIT) << 10;
    sig_b = (sig_b | IMPLICIT) << 11;
    sig = mul_high_jam(sig_a, sig_b);

    if (sig < 0x4000000000000000ULL) {
        exp--;
        sig <<= 1;
    }

    return from_bits(round_pack(sign, exp, sig));
}

double __divdf3(double a, double b) {
    uint64_t ua = to_bits(a), ub = to_bits(b);
    int sign = SIGN(ua) ^ SIGN(ub);
    int exp_a = EXP(ua), exp_b = EXP(ub);
    uint64_t sig_a = MANT(ua), sig_b = MANT(ub);
    uint64_t q, rem;
    int exp, i;

    if (exp_a == 0x7ff) {
        if (sig_a) {
            return from_bits(propagate_nan(ua, ub));
        }
        if (exp_b == 0x7ff) {
            // inf / inf is invalid
            return from_bits(sig_b ? propagate_nan(ua, ub) : DEFAULT_NAN);
        }
        return from_bits(pack(sign, 0x7ff, 0));
    }
    if (exp_b == 0x7ff) {
        return from_bits(sig_b ? propagate_nan(ua, ub) : pack(sign, 0, 0));
    }

    if (exp_b == 0) {
        if (sig_b == 0) {
            // 0 / 0 is invalid; x / 0 is infinity
            return from_bits(((exp_a | sig_a) == 0) ? DEFAULT_NAN : pack(sign, 0x7ff, 0));
        }
        sig_b = norm_denormal(sig_b, &exp_b);
    }
    if (exp_a == 0) {
        if (sig_a == 0) {
            return from_bits(pack(sign, 0, 0));
        }
        sig_a = norm_denormal(sig_a, &exp_a);
    }

    exp = exp_a - exp_b + 0x3fe;
    sig_a |= IMPLICIT;
    sig_b |= IMPLICIT;
    if (sig_a < sig_b) {
        exp--;
        sig_a <<= 1;
    }

    // sig_a / sig_b is now in [1, 2): long division gives it to 63 bits
    rem = sig_a;
    q = 0;
    for (i = 0; i < 63; i++) {
        q <<= 1;
        if (rem >= sig_b) {
            rem -= sig_b;
            q |= 1;
        }
        rem <<= 1;
    }

    return from_bits(round_pack(sign, exp, q | (rem != 0)));
}

double __negdf2(double a) {
    return from_bits(to_bits(a) ^ (1ULL << 63));
}

/*
 * Compare: -1, 0 or 1 for a < b, a == b, a > b, or unordered if either is
 * a NaN. (+0 and -0 are equal.)
 */
static int compare(uint64_t a, uint64_t b, int unordered) {
    if (IS_NAN(a) || IS_NAN(b)) {
        return unordered;
    }

    if (((a | b) << 1) == 0) {
        return 0;
    }

    if (SIGN(a) != SIGN(b)) {
        return SIGN(a) ? -1 : 1;
    }

    if (a == b) {
        return 0;
    }

    // Same sign: the bigger magnitude is bigger if positive
    return ((a > b) ^ SIGN(a)) ? 1 : -1;
}

/*
 * What each returns when either is a NaN makes its test false: gcc uses
 * __eqdf2() == 0, __ltdf2() < 0, __gedf2() >= 0 and so on.
 */
int __cmpdf2(double a, double b) { return compare(to_bits(a), to_bits(b), 1); }
int __eqdf2(double a, double b)  { return compare(to_bits(a), to_bits(b), 1); }
int __nedf2(double a, double b)  { return compare(to_bits(a), to_bits(b), 1); }
int __ltdf2(double a, double b)  { return compare(to_bits(a), to_bits(b), 1); }
int __ledf2(double a, double b)  { return compare(to_bits(a), to_bits(b), 1); }
int __gtdf2(double a, double b)  { return compare(to_bits(a), to_bits(b), -1); }
int __gedf2(double a, double b)  { return compare(to_bits(a), to_bits(b), -1); }

// float: as double, then rounded (see the top of the file)

float __addsf3(float a, float b) {
    return __truncdfsf2(__adddf3(__extendsfdf2(a), __extendsfdf2(b)));
}

float __subsf3(float a, float b) {
    return __truncdfsf2(__subdf3(__extendsfdf2(a), __extendsfdf2(b)));
}

float __mulsf3(float a, float b) {
    return __truncdfsf2(__muldf3(__extendsfdf2(a), __extendsfdf2(b)));
}

float __divsf3(float a, float b) {
    return __truncdfsf2(__divdf3(__extendsfdf2(a), __extendsfdf2(b)));
}

float __negsf2(float a) {
    fbits_t b;

    b.f = a;
    b.u ^= 0x80000000U;
    return b.f;
}

// A float widens to double exactly, so compare as doubles
static int compare_sf(float a, float b, int unordered) {
    return compare(to_bits(__extendsfdf2(a)), to_bits(__extendsfdf2(b)), unordered);
}

int __cmpsf2(float a, float b) { return compare_sf(a, b, 1); }
int __eqsf2(float a, float b)  { return compare_sf(a, b, 1); }
int __nesf2(float a, float b)  { return compare_sf(a, b, 1); }
int __ltsf2(float a, float b)  { return compare_sf(a, b, 1); }
int __lesf2(float a, float b)  { return compare_sf(a, b, 1); }
int __gtsf2(float a, float b)  { return compare_sf(a, b, -1); }
int __gesf2(float a, float b)  { return compare_sf(a, b, -1); }

// float <-> 64-bit integers

int64_t __fixsfdi(float f) {
    return __fixdfdi(__extendsfdf2(f));
}

uint64_t __fixunssfdi(float f) {
    return __fixunsdfdi(__extendsfdf2(f));
}

/*
 * Going through double would round twice, which can be wrong. So a value
 * over 53 bits is first shifted down to 53, keeping a sticky bit for
 * anything dropped: that converts to double exactly and rounds to the
 * right float, whose exponent then just needs the shift adding back.
 */
static float u64_to_float(uint64_t v) {
    fbits_t r;
    int shift = 0;

    if (v >> 53) {
        shift = 11 - clz64(v);
        v = shift_right_jam(v, shift);
    }

    r.f = __truncdfsf2(__floatundidf(v));
    r.u += (uint32_t)shift << 23;      // a normal float, far from overflow

    return r.f;
}

float __floatundisf(uint64_t v) {
    return u64_to_float(v);
}

float __floatdisf(int64_t v) {
    fbits_t r;

    if (v >= 0) {
        return __floatundisf((uint64_t)v);
    }

    r.f = __floatundisf(0ULL - (uint64_t)v);
    r.u ^= 0x80000000U;
    return r.f;
}

// x to an integer power, by repeated squaring (as libgcc does it)

double __powidf2(double x, int n) {
    unsigned m = (n < 0) ? 0U - (unsigned)n : (unsigned)n;
    double y = (m & 1) ? x : from_bits(0x3ff0000000000000ULL);

    while (m >>= 1) {
        x = __muldf3(x, x);
        if (m & 1) {
            y = __muldf3(y, x);
        }
    }

    return (n < 0) ? __divdf3(from_bits(0x3ff0000000000000ULL), y) : y;
}

float __powisf2(float x, int n) {
    unsigned m = (n < 0) ? 0U - (unsigned)n : (unsigned)n;
    fbits_t one;
    float y;

    one.u = 0x3f800000U;
    y = (m & 1) ? x : one.f;

    while (m >>= 1) {
        x = __mulsf3(x, x);
        if (m & 1) {
            y = __mulsf3(y, x);
        }
    }

    return (n < 0) ? __divsf3(one.f, y) : y;
}
