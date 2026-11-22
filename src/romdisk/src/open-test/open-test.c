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
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>

#define NUM_OPENS 25

void main(void) {
    int fds[NUM_OPENS];
    int ok = NUM_OPENS;

    for (int i=0; i<NUM_OPENS; i++) {
        printf("%2d: ", i);

        fds[i] = open("/rom0/README", O_RDONLY);
        if (fds[i] == NOT_OK) {
            printf("failed with errno=%d\n", errno);
            ok = i+1;
            i = NUM_OPENS;
        }
        else {
            printf("ok\n");
        }
    }

    for (int i=0; i<ok; i++) {
        close(fds[i]);
    }
}