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
#include <limits.h>
#include <errno.h>

unsigned long long __strtonum(const char *nptr, char **endptr, int base, int *neg, int *overflow);

/*
 * C99 strtoll(). Out of range values give LLONG_MIN or LLONG_MAX, with
 * errno set to ERANGE. (See strtonum.c for the parsing.)
 */
long long strtoll(const char *nptr, char **endptr, int base) {
    int neg, overflow;
    unsigned long long mag = __strtonum(nptr, endptr, base, &neg, &overflow);

    if (base == 1 || base < 0 || base > 36) {
        errno = EINVAL;
        return 0;
    }

    if (neg) {
        if (overflow || (mag > (unsigned long long)LLONG_MAX + 1)) {
            errno = ERANGE;
            return LLONG_MIN;
        }
        return (long long)(0 - mag);
    }

    if (overflow || (mag > (unsigned long long)LLONG_MAX)) {
        errno = ERANGE;
        return LLONG_MAX;
    }

    return (long long)mag;
}
