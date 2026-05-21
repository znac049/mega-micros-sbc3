#include <stdio.h>
#include <duart.h>

int printf(const char *format, ...) {
	int res;
	va_list args;
	char buffer[512];

	va_start(args, format);
	res = vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);

	for (int i=0; buffer[i]; i++) {
		_putchar(buffer[i]);
	}

	return res;
}