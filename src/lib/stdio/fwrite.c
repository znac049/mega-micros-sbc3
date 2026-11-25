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
#include <unistd.h>
#include <errno.h>

size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream) {
    const char *p = ptr;
    size_t total, done = 0;

    if ((stream == NULL) || (stream->is_open == 0)) {
        errno = EBADF;
        return 0;
    }

    if ((size == 0) || (nmemb == 0)) {
        return 0;
    }

    total = size * nmemb;

    while (done < total) {
        ssize_t n = write(stream->fd, (void *)(p + done), total - done);

        if (n <= 0) {
            stream->error = YES;
            break;
        }

        done += n;
    }

    return done / size;
}
