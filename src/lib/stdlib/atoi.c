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

int atoi(const char *str) {
    if (str == NULL) {
        return 0; // Null pointer safety
    }

    // Skip leading whitespace
    while (isspace((unsigned char)*str)) {
        str++;
    }

    // Handle optional sign
    int sign = 1;
    if (*str == '+' || *str == '-') {
        if (*str == '-') {
            sign = -1;
        }
        str++;
    }

    long result = 0; // Use long to detect overflow before casting to int

    // Convert digits
    while (isdigit((unsigned char)*str)) {
        int digit = *str - '0';

        // Check for overflow before multiplying/adding
        if (result > (LONG_MAX - digit) / 10) {
            return (sign == 1) ? INT_MAX : INT_MIN;
        }

        result = result * 10 + digit;
        str++;
    }

    return (int)(sign * result);
}

int old_atoi(const char *str) {
	int res = 0;
    int neg = 0;

    // Skip leading whitespace
	while (*str == ' ' || *str == '\t') {
        str++;
    }

	if (*str == '-') {
		neg = 1;
		str++;
	}

	for (char ch= *str; ch >= '0' && ch <= '9'; ch=*str++) {
		res *= 10;
		res += (ch - '0');
	}

	return neg?-res:res;
}
