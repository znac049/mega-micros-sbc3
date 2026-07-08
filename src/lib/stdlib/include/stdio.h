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

// Stream object
typedef struct file {
    int fd;
    uint8_t is_open;
} FILE;

#define STREAM_TABLE_SIZE 16

#define FHAND_DUART 1

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

int char_available(void);
int fclose(FILE *stream);
int fflush(FILE *stream);
int fgetc(FILE *stream);
char *fgets(char *s, int size, FILE *stream);
extern FILE *fopen(const char *pathname, const char *mode);
int fprintf(FILE *stream, const char *fmt, ...);
int fputc(int c, FILE *stream);
int fputs(const char *s, FILE *stream);
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t fwrite(void *ptr, size_t size, size_t nmemb, FILE *stream);
int getchar(void);
char *gets(char *s);
int printf(const char *format, ...);
int putchar(int c);
int puts(const char *s);
int snprintf(char *buffer, size_t n, const char *fmt, ...);
int sscanf(const char *str, const char *format, ...);
int vfprintf(FILE *stream, const char *format, va_list ap);
int vsnprintf(char *str, size_t size, const char *format, va_list ap);
int vsscanf(const char *str, const char *format, va_list ap);