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
 * Software float <-> integer conversions, replacing libgcc's.
 *
 * The m68k-linux-gnu toolchain's libgcc is built for a 68881/68882: its
 * add, subtract, multiply and divide are done in software, but these
 * conversions use FPU instructions and return their result in fp0. Code
 * built with -msoft-float (all of ours) expects the result in d0/d1, so
 * it gets garbage - (double)1 comes out as 0x4000000000000016 - and on a
 * board without an FPU each call takes an F-line exception instead.
 *
 * libmega is linked before libgcc, so these are used in place of its own.
 * Everything libgcc's fpgnulib.o defines is here, so that object is
 * never pulled in (it would clash).
 * They only shuffle bits: any int/double conversion in here would call
 * the very functions being defined.
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

#define DMANT_BITS  52
#define DEXP_BIAS   1023
#define DMANT_MASK  ((1ULL << DMANT_BITS) - 1)
#define FMANT_BITS  23
#define FEXP_BIAS   127
#define FMANT_MASK  ((1U << FMANT_BITS) - 1)

double __floatundidf(uint64_t v);
double __floatdidf(int64_t v);
double __floatsidf(int32_t v);
double __floatunsidf(uint32_t v);
int32_t __fixdfsi(double d);
int64_t __fixdfdi(double d);
uint64_t __fixunsdfdi(double d);
uint32_t __fixunsdfsi(double d);
double __extendsfdf2(float f);
float __truncdfsf2(double d);
float __floatsisf(int32_t v);
float __floatunsisf(uint32_t v);
int32_t __fixsfsi(float f);
uint32_t __fixunssfsi(float f);
int __unorddf2(double a, double b);
int __unordsf2(float a, float b);

static int top_bit(uint64_t v) {
    int n = 63;

    while (!(v & (1ULL << n))) {
        n--;
    }

    return n;
}

// A magnitude and a sign as a double, rounded to nearest (ties to even)
static double make_double(uint64_t v, int negative) {
    dbits_t r;
    uint64_t mant;
    int msb, exp;

    if (v == 0) {
        r.u = (uint64_t)negative << 63;
        return r.d;
    }

    msb = top_bit(v);
    exp = msb + DEXP_BIAS;

    if (msb <= DMANT_BITS) {
        mant = v << (DMANT_BITS - msb);
    }
    else {
        int shift = msb - DMANT_BITS;
        uint64_t rem = v & ((1ULL << shift) - 1);
        uint64_t half = 1ULL << (shift - 1);

        mant = v >> shift;
        if ((rem > half) || ((rem == half) && (mant & 1))) {
            mant++;
            if (mant == (1ULL << (DMANT_BITS + 1))) {
                mant >>= 1;
                exp++;
            }
        }
    }

    r.u = ((uint64_t)negative << 63) | ((uint64_t)exp << DMANT_BITS) | (mant & DMANT_MASK);

    return r.d;
}

double __floatundidf(uint64_t v) {
    return make_double(v, 0);
}

double __floatdidf(int64_t v) {
    return (v < 0)?make_double(0ULL - (uint64_t)v, 1):make_double((uint64_t)v, 0);
}

double __floatsidf(int32_t v) {
    return (v < 0)?make_double(0ULL - (uint64_t)(int64_t)v, 1):make_double((uint64_t)v, 0);
}

double __floatunsidf(uint32_t v) {
    return make_double(v, 0);
}

/*
 * The integer part of |d| (truncated), if it fits in 64 bits; *big is set
 * if it doesn't (including infinity and NaN).
 */
static uint64_t magnitude(double d, int *negative, int *big) {
    dbits_t b;
    int exp;
    uint64_t mant;

    b.d = d;
    *negative = (int)(b.u >> 63);
    *big = 0;
    exp = (int)((b.u >> DMANT_BITS) & 0x7ff) - DEXP_BIAS;

    if (exp < 0) {
        return 0;
    }

    if (exp > 63) {
        *big = 1;
        return 0;
    }

    mant = (b.u & DMANT_MASK) | (1ULL << DMANT_BITS);

    return (exp >= DMANT_BITS)?(mant << (exp - DMANT_BITS)):(mant >> (DMANT_BITS - exp));
}

// Out of range is undefined in C: these saturate
int64_t __fixdfdi(double d) {
    int negative, big;
    uint64_t m = magnitude(d, &negative, &big);

    if (big || (m > (negative?(1ULL << 63):((1ULL << 63) - 1)))) {
        return negative?(int64_t)(1ULL << 63):(int64_t)((1ULL << 63) - 1);
    }

    return negative?(int64_t)(0ULL - m):(int64_t)m;
}

int32_t __fixdfsi(double d) {
    int64_t v = __fixdfdi(d);

    if (v > 0x7fffffffLL) {
        return 0x7fffffff;
    }

    if (v < -0x80000000LL) {
        return (int32_t)0x80000000U;
    }

    return (int32_t)v;
}

uint64_t __fixunsdfdi(double d) {
    int negative, big;
    uint64_t m = magnitude(d, &negative, &big);

    if (negative) {
        return 0;
    }

    return big?~0ULL:m;
}

uint32_t __fixunsdfsi(double d) {
    uint64_t m = __fixunsdfdi(d);

    return (m > 0xffffffffULL)?0xffffffffU:(uint32_t)m;
}

double __extendsfdf2(float f) {
    fbits_t a;
    dbits_t r;
    uint64_t sign;
    int exp;
    uint32_t mant;

    a.f = f;
    sign = (uint64_t)(a.u >> 31) << 63;
    exp = (int)((a.u >> FMANT_BITS) & 0xff);
    mant = a.u & FMANT_MASK;

    if (exp == 0xff) {
        // Infinity or NaN (keeping the NaN's payload)
        r.u = sign | (0x7ffULL << DMANT_BITS) | ((uint64_t)mant << (DMANT_BITS - FMANT_BITS));
    }
    else if (exp == 0) {
        if (mant == 0) {
            r.u = sign;
        }
        else {
            // Denormal float: normal as a double
            int shift = FMANT_BITS - top_bit(mant);

            mant = (mant << shift) & FMANT_MASK;
            exp = 1 - shift;
            r.u = sign | ((uint64_t)(exp - FEXP_BIAS + DEXP_BIAS) << DMANT_BITS) |
                  ((uint64_t)mant << (DMANT_BITS - FMANT_BITS));
        }
    }
    else {
        r.u = sign | ((uint64_t)(exp - FEXP_BIAS + DEXP_BIAS) << DMANT_BITS) |
              ((uint64_t)mant << (DMANT_BITS - FMANT_BITS));
    }

    return r.d;
}

float __truncdfsf2(double d) {
    dbits_t a;
    fbits_t r;
    uint32_t sign;
    int exp;
    uint64_t mant;

    a.d = d;
    sign = (uint32_t)(a.u >> 63) << 31;
    exp = (int)((a.u >> DMANT_BITS) & 0x7ff);
    mant = a.u & DMANT_MASK;

    if (exp == 0x7ff) {
        // Infinity, or NaN (kept quiet)
        r.u = sign | (0xffU << FMANT_BITS) | ((mant != 0)?(0x400000U | (uint32_t)(mant >> 29)):0);
        return r.f;
    }

    if ((exp == 0) && (mant == 0)) {
        r.u = sign;
        return r.f;
    }

    // Full significand, with its implicit 1, and the float's exponent
    mant |= (exp != 0)?(1ULL << DMANT_BITS):0;
    exp = ((exp != 0)?exp:1) - DEXP_BIAS + FEXP_BIAS;

    {
        // Drop 29 bits (more for a denormal result), rounding to nearest even
        int shift = (DMANT_BITS - FMANT_BITS) + ((exp < 1)?(1 - exp):0);
        uint64_t kept, rem, half;

        if (shift > 60) {
            r.u = sign;             // far too small: zero
            return r.f;
        }

        kept = mant >> shift;
        rem = mant & ((1ULL << shift) - 1);
        half = 1ULL << (shift - 1);

        if ((rem > half) || ((rem == half) && (kept & 1))) {
            kept++;
        }

        if (exp < 1) {
            // Denormal (rounding may have made it the smallest normal)
            r.u = sign | (uint32_t)kept;
            return r.f;
        }

        if (kept == (1ULL << (FMANT_BITS + 1))) {
            kept >>= 1;
            exp++;
        }

        if (exp >= 0xff) {
            r.u = sign | (0xffU << FMANT_BITS);     // overflow: infinity
            return r.f;
        }

        r.u = sign | ((uint32_t)exp << FMANT_BITS) | ((uint32_t)kept & FMANT_MASK);
    }

    return r.f;
}

float __floatsisf(int32_t v) {
    return __truncdfsf2(__floatsidf(v));    // exact as a double first
}

float __floatunsisf(uint32_t v) {
    return __truncdfsf2(__floatunsidf(v));
}

int32_t __fixsfsi(float f) {
    return __fixdfsi(__extendsfdf2(f));
}

uint32_t __fixunssfsi(float f) {
    return __fixunsdfsi(__extendsfdf2(f));
}

// Is either a NaN? (all ones exponent, non-zero mantissa)
int __unorddf2(double a, double b) {
    dbits_t x, y;

    x.d = a;
    y.d = b;

    return (((x.u << 1) > (0x7ffULL << 53)) || ((y.u << 1) > (0x7ffULL << 53)))?1:0;
}

int __unordsf2(float a, float b) {
    fbits_t x, y;

    x.f = a;
    y.f = b;

    return (((x.u << 1) > (0xffU << 24)) || ((y.u << 1) > (0xffU << 24)))?1:0;
}
