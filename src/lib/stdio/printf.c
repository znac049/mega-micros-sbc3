#include <stdio.h>
#include <duart.h>

int printf(const char *format, ...) {
	int res;
	va_list args;
	char buffer[512];
	char *s = buffer;

	va_start(args, format);
	res = vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);

	while (*s) {
		putchar(*s++);
	}

	return res;
}