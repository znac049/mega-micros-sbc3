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

#include <math.h>
#include <ctype.h>

// 2^n as a double, for -1022 <= n <= 1023
static double pow2(int n) {
    union {
        double d;
        uint64_t bits;
    } u;

    u.bits = (uint64_t)(n + 1023) << 52;

    return u.d;
}

// x * 2^exp
double ldexp(double x, int exp) {
    if ((x == 0.0) || (x != x) || (x - x != 0.0)) {
        return x;                   // zero, NaN or infinity
    }

    while (exp > 1023) {
        x *= pow2(1023);
        exp -= 1023;
        if (x - x != 0.0) {
            return x;               // overflowed to infinity
        }
    }

    while (exp < -1022) {
        x *= pow2(-1022);
        exp += 1022;
        if (x == 0.0) {
            return x;               // underflowed to zero
        }
    }

    return x * pow2(exp);
}
