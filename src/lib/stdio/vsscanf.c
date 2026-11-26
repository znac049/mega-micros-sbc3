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
#include "scan.h"

// sscanf() reads a string (see scan.c)

static int string_get(void *ctx) {
    const unsigned char **s = (const unsigned char **)ctx;

    if (**s == '\0') {
        return EOF;
    }

    return *(*s)++;
}

static void string_unget(int c, void *ctx) {
    const unsigned char **s = (const unsigned char **)ctx;

    (void)c;
    (*s)--;
}

int vsscanf(const char *str, const char *format, va_list ap) {
    const unsigned char *s = (const unsigned char *)str;
    scan_src_t src;

    src.get = string_get;
    src.unget = string_unget;
    src.ctx = &s;
    src.count = 0;

    return __scan(&src, format, ap);
}
