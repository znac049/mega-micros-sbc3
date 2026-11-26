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

#pragma once

#include <ctype.h>
#include <stddef.h>

#define EXIT_SUCCESS    0
#define EXIT_FAILURE    1

int atoi(const char *nptr);
long atol(const char *nptr);
void *calloc(size_t nmemb, size_t size);
char *getenv(const char *name);
char *itoa(unsigned int value, char *string, int base);
void _init_heap(void);
char *itoa_padded(unsigned int value, char *string, int base, char width, char zeropad, char is_signed, char ten_char);
void *malloc(size_t size);
void free(void *ptr);
void qsort(void *base, size_t nmemb, size_t size, int (*compar)(const void *, const void *));
int rand(void);
void *realloc(void *ptr, size_t size);
char *realpath(const char *path, char *resolved_path);
void srand(unsigned int seed);
double strtod(const char *nptr, char **endptr);
long strtol(const char *nptr, char **endptr, int base);
long long strtoll(const char *nptr, char **endptr, int base);
unsigned long strtoul(const char *nptr, char **endptr, int base);
unsigned long long strtoull(const char *nptr, char **endptr, int base);