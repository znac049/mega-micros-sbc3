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

#include <stdarg.h>
#include <ctype.h>

typedef struct file {
    int fd;
} FILE;

extern FILE __stdin;
extern FILE __stdout;
extern FILE __stderr;

#define	stdin  (&__stdin)
#define	stdout (&__stdout)
#define	stderr (&__stderr)

#define EOF -1

int char_available(void);
int fclose(FILE *stream);
int fflush(FILE *stream);
int fgetc(FILE *stream);
char *fgets(char *s, int size, FILE *stream);
int fputc(int c, FILE *stream);
int fputs(const char *s, FILE *stream);
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t fwrite(void *ptr, size_t size, size_t nmemb, FILE *stream);
int getchar(void);
char *gets(char *s);
int printf(const char *format, ...);
int putchar(int c);
int puts(const char *s);
int sscanf(const char *str, const char *format, ...);
int vfprintf(FILE *stream, const char *format, va_list ap);
int vsnprintf(char *str, size_t size, const char *format, va_list ap);
int vsscanf(const char *str, const char *format, va_list ap);