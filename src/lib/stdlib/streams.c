
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <machine.h>

FILE *__stdin = &__console;
FILE *__stdout = &__console;
FILE *__stderr = &__console;
FILE __console = { DEVTYPE_CHAR, 0, "CON:", &xr68681_device };
FILE __aux = { DEVTYPE_CHAR, 1, "AUX:", &xr68681_device };

void init_streams(void) {

}

FILE *fopen(const char *pathname, const char *mode) {
	// Special filenames
	if (strcasecmp(pathname, "CON:") == 0) {
		return &__console
	}
	else if (strcasecmp(pathname, "AUX:") == 0) {
		return &__aux;
	}

	return NULL;
}