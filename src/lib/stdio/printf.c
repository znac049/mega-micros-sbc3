#include <stdio.h>

int printf(const char *format, ...) {
	int res;
	va_list args;

	va_start(args, format);
	res = vprintf(format, args);
	va_end(args);

	return res;
}
