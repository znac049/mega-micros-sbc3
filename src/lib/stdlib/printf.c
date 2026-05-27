#include <stdio.h>
#include <duart.h>

int printf(const char *format, ...) {
	int res;
	va_list args;
	char buffer[512];

	va_start(args, format);
	res = vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);

	fputs(buffer, stdout);

	return res;
}