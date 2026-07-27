
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int vsnprintf(char *str, size_t size, const char *format, va_list ap)
{
	int len;
	int i = 0;

	for (int j = 0; i < size - 1 && format[j] != '\0'; j++) {
		if (format[j] == '%') {
			char type;
			char width = 0;
			char zeropad = 0;
			char rightpad = 0;
			char numprefix = 0;
			char length = sizeof(unsigned int);

			// left or right padding?
			if (format[j + 1] == '-') {
				rightpad = 1;
				j += 1;
			}
			
			// Should it be zero padding or spaces
			if (format[j + 1] == '0') {
				zeropad = 1;
				j += 1;
			} else if (format[j + 1] == '#') {
				numprefix = 1;
				j += 1;
			}

			// Parse the width to pad the data to
			if (isdigit(format[j + 1])) {
				char *endptr;
				width = strtol(&format[j + 1], &endptr, 10);
				j += endptr - &format[j] - 1;
			}

			// TODO parse out the precision

			// Parse out length of data (for numbers mostly)
			if (format[j + 1] == 'h') {
				if (format[j + 2] == 'h') {
					length = sizeof(char);
					j += 2;
				} else {
					length = sizeof(short);
					j += 1;
				}
			} else if (format[j + 1] == 'l') {
				if (format[j + 2] == 'l') {
					length = sizeof(long long);
					j += 2;
				} else {
					length = sizeof(long);
					j += 1;
				}
			}

			// Parse the data type
			type = format[++j];

			// Fetch the data and format it accordingly
			switch (type) {
			    case 's': {
					const char *s;

					s = va_arg(ap, const char *);
					len = strlen(s);

					if (width && (len < width)) {
						if (!rightpad) {
							// Padding before the string
							for (int x=len; x<width; x++, i++) {
								str[i] = ' ';
							}
						}
					}

					strncpy(&str[i], s, size - i - 1);
					i += len;

					if (width && (len < width)) {
						if (rightpad) {
							// Padding after the string
							for (int x=len; x<width; x++, i++) {
								str[i] = ' ';
							}
						}
					}
					break;
			    }

			    case 'i':
					type = 'd';
			    case 'p':
			    case 'd':
			    case 'u':
			    case 'o':
			    case 'x':
			    case 'X': {
					// Extract the data from ap based on the requested length
					unsigned long long d;
					if (length <= sizeof(int)) {
						d = va_arg(ap, unsigned int);
					} else {
						d = va_arg(ap, unsigned long long);
					}

					// Determine what base to display the number in
					int radix = 10;

					if (type == 'x' || type == 'X' || type == 'p') {
						if (numprefix && d) {
							str[i++] = '0';
							str[i++] = (type == 'x')?'x':'X';
						}
						radix = 16;
					} else if (type == 'o') {
						if (numprefix && d)
							str[i++] = '0';
						radix = 8;
					}

					itoa_padded(d, &str[i], radix, width, zeropad, type == 'd' ? 1 : 0, type == 'x' ? 'a' : 'A');
					i += strlen(&str[i]);
					break;
			    }

				case 'f': {
					// Total bodge - needs to be reworked.
					double d;
					long intpart;
					long floatpart;

					d = va_arg(ap, double);
					intpart = (long int)d;
					floatpart = (d - intpart) * 100000000;
					itoa_padded(intpart, &str[i], 10, width, zeropad, 1, 0);
					i += strlen(&str[i]);
					str[i++] = '.';
					itoa_padded(floatpart, &str[i], 10, width, 1, 1, 0);
					break;
				}

				case 'c': {
					int c;

					c = va_arg(ap, int);
					str[i++] = (char) c;
					break;
				}
				
				default:
					break;
			}
		} else {
			str[i++] = format[j];
		}
	}
	str[i] = '\0';

	return i;
}

