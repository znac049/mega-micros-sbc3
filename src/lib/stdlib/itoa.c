#include <stdlib.h>

char *itoa_padded(unsigned int value, char *string, int base, char width, char zeropad, char is_signed) {
	char c;
	int i = 0;
	int j = 0;
	int sign = 0;

	if (is_signed && ((signed int) value) < 0) {
		sign = 1;
		value = -value;
	}

	do {
		string[i] = (value % base) + '0';
		if (string[i] > '9')
			string[i] += 0x07;
		value /= base;
		i++;
	} while (value > 0);

	if (sign) {
		string[i++] = '-';
    }

	for (; i < width; i++) {
		string[i] = zeropad ? '0' : ' ';
    }

	string[i--] = '\0';

	for (; j < i; j++, i--) {
		c = string[j];
		string[j] = string[i];
		string[i] = c;
	}

	return string;
}

char *itoa(unsigned int value, char *string, int base) {
	return itoa_padded(value, string, base, 0, 0, base == 10 ? 1 : 0);
}

