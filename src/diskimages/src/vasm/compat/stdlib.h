/* The library's <stdlib.h>, plus the EXIT_ codes it lacks */
#include_next <stdlib.h>
#include <stddef.h>

#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS    0
#define EXIT_FAILURE    1
#endif
