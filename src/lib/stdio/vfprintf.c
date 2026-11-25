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
#include <stddef.h>
#include <stdlib.h>

#define SMALL_STR   256

/*
 * Format into a buffer on the stack, or into one from malloc() if the
 * output is longer than that.
 */
int vfprintf(FILE *stream, const char *format, va_list args) {
    char buffer[SMALL_STR];
    va_list copy;
    int res;

    va_copy(copy, args);
    res = vsnprintf(buffer, sizeof(buffer), format, args);

    if (res < (int)sizeof(buffer)) {
        if ((res > 0) && (fwrite(buffer, 1, res, stream) != (size_t)res)) {
            res = EOF;
        }
    }
    else {
        char *big = malloc(res + 1);

        if (big == NULL) {
            res = EOF;
        }
        else {
            vsnprintf(big, res + 1, format, copy);
            if (fwrite(big, 1, res, stream) != (size_t)res) {
                res = EOF;
            }
            free(big);
        }
    }

    va_end(copy);

    return res;
}
