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

#include <ctype.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <nonstd.h>
#include <machine.h>

#include "micromon.h"

void handle_cat_command(int argc, char *argv[]) {
    FILE *fd;
    int ch;

    if (argc != 2) {
        printf("usage: cat <filename>\n");
        return;
    }

    fd = fopen(argv[1], "r");
    if (fd == NULL) {
        printf("Couldn't open file '%s'\n", argv[1]);
        return;
    }

    while ((ch = fgetc(fd)) != EOF) {
        putchar(ch);
    }

    fclose(fd);
    putchar('\n');
}