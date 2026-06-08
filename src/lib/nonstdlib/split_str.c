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

#include <string.h>

int split_str(char *s, char sep, char *bits[], int max_bits) {
    int bit_num = 0;
    char *next_sep = strchr(s, sep);

    if (sep == 0) {
        return -1;
    }

    max_bits--;

    while ((next_sep != NULL) && (bit_num < max_bits)) {
        bits[bit_num++] = s;
        *next_sep++ = EOS;

        s = next_sep;
        while (*s == sep) {
            s++;
        }

        if (*s == EOS) {
            return bit_num;
        }

        next_sep = strchr(s, sep);
    }

    bits[bit_num++] = s;

    return bit_num;
}