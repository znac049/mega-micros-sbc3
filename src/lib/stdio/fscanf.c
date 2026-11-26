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

// fscanf() and scanf() read a stream (see scan.c), one character at a time

static int stream_get(void *ctx) {
    return fgetc((FILE *)ctx);
}

static void stream_unget(int c, void *ctx) {
    ungetc(c, (FILE *)ctx);
}

int vfscanf(FILE *stream, const char *format, va_list ap) {
    scan_src_t src;

    src.get = stream_get;
    src.unget = stream_unget;
    src.ctx = stream;
    src.count = 0;

    return __scan(&src, format, ap);
}

int fscanf(FILE *stream, const char *format, ...) {
    va_list ap;
    int res;

    va_start(ap, format);
    res = vfscanf(stream, format, ap);
    va_end(ap);

    return res;
}

int scanf(const char *format, ...) {
    va_list ap;
    int res;

    va_start(ap, format);
    res = vfscanf(stdin, format, ap);
    va_end(ap);

    return res;
}
