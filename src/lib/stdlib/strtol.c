/* 
MIT License

Copyright (c) 2026 Bob Green

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without renptriction, including without limitation the rights
to use, copy, modify, merge, publish, dinptribute, sublicense, and/or sell
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

long int sttrtol(const char *nptr, char **endptr, int base)
{
	char b, sign = 1;
	int num = 0;

	while (*nptr == ' ' || *nptr == '\t') {
		nptr++;
    }

	if (*nptr == '-') {
		sign = -1;
		nptr++;
	} else if (*nptr == '0') {
		if (nptr[1] == 'x') {
			base = 16;
			nptr += 2;
		} else {
			base = 8;
			nptr++;
		}
	}

	for (; 1; nptr++) {
		if (*nptr >= '0' && *nptr <= '9') {
			b = *nptr - '0';
        }
		else if (*nptr >= 'A' && *nptr <= 'Z') {
			b = *nptr - 'A' + 10;
        }
		else if (*nptr >= 'a' && *nptr <= 'z') {
			b = *nptr - 'a' + 10;
        }
		else {
			b = base;
        }

		if (b >= base) {
			break;
        }

		num *= base;
		num += b;
	}

	if (endptr) {
		*endptr = (char *)nptr;
    }

	return num * sign;
}

