
#include <stdio.h>

#define MAX_STR	400

int vfprintf(FILE *stream, const char *format, va_list args)
{
	int r;
	char buffer[MAX_STR];

	r = vsnprintf(buffer, MAX_STR, format, args);
	fputs(buffer, stream);

	return r;
}