#include <stdio.h>

int printf(const char *format, ...) {
	int r;
	va_list args;

	va_start(args, format);
	r = vfprintf(stdout, format, args);
	va_end(args);

	return r;
}