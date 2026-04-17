
#include <stdio.h>

int fprintf(FILE *stream, const char *fmt, ...) {
	int res;
	va_list args;

	va_start(args, fmt);
	res = vfprintf(stream, fmt, args);
	va_end(args);

	return res;
}

