#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

int vsscanf(const char *str, const char *format, va_list ap)
{
	int i = 0;
	int count = 0;

	for (int j = 0; format[j] != '\0' && str[i] != '\0'; j++) {
		if (format[j] == '%') {
			char type;
			char width = 0;
			char ignore = 0;
			char length = sizeof(unsigned int);

			// Should it be ignored or saved
			if (format[j + 1] == '*') {
				ignore = 1;
				j += 1;
			}

			// Parse the width to read
			if (isdigit(format[j + 1])) {
				char *endptr;
				width = strtol(&format[j + 1], &endptr, 10);
				j += endptr - &format[j] - 1;
			}

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

			switch (type) {
			    case 'i':
			    case 'd':
			    case 'x':
			    case 'X': {
				char *endptr;
				register int number = strtol(&str[i], &endptr, type == 'd' || type == 'i' ? 10 : 16);
				if (!ignore) {
					void *ptr = va_arg(ap, void *);
					switch (length) {
					    case 2:
						*((unsigned short *) ptr) = number;
						break;
					    case 4:
						*((unsigned int *) ptr) = number;
						break;
					    case 8:
						*((unsigned long long *) ptr) = number;
						break;
					    default:
						// If there's an invalid character, return immediately
						// with the count of valid reads up until this point
						return count;
					}
				}
				i += (endptr - &str[i]);
				break;
			    }
			    case 's': {
				if (ignore) {
					for (; !isspace(str[i]); i++) { }
				} else {
					char *buffer = va_arg(ap, char *);
					for (; !isspace(str[i]); i++)
						*buffer++ = str[i];
					*buffer = '\0';
				}
				break;
			    }
			    case 'c': {
				if (!width)
					width = 1;

				if (ignore) {
					i += width;
				} else {
					char *buffer = va_arg(ap, char *);
					for (; width; width--, i++)
						*buffer++ = str[i];
				}
				break;
			    }
			    default: {
				// If there's an invalid character, return immediately
				// with the count of valid reads up until this point
				return count;
			    }
			}
			// Increment the number of arguments read successfully
			count++;
		} else if (isspace(format[j])) {
			// Read any amount of whitespace from the input string
			for (; isspace(str[i]); i++) { }
		} else if (format[j] != str[i++]) {
			// For any other non-matching character, then fail and return
			return count;
		}
	}
	return count;
}

