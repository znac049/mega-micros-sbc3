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

char *fgets(char *s, int size, FILE *stream) {
    int ch;
    int i = 0;

    while (i+1 < size) {
        ch = fgetc(stream);
        if (ch == -1) {
            if (i == 0) {
                return NULL;
            }
            else {
                s[i] = EOS;
                return s;
            }
        }

        switch(ch) {
            case '\r': case '\n':
                s[i++] = '\n';
                s[i] = EOS;
                fputc('\n', stream);

                return s;

            case BS:
                if (i) {
                    fputc(BS, stream);
                    fputc(' ', stream);
                    fputc(BS, stream);
                    i--;
                }
                break;

            default:
                fputc(ch, stream);
                s[i++] = ch;
                break;
        }
    }

    s[i] = EOS;

    return s;
}
