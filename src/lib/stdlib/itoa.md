% itoa(3) Version 1.0 | Library Functions Manual
***

## NAME

**itoa** — convert integer to string

### LIBRARY

libmega C library (-lmega)

### SYNOPSIS

```
#include <stdlib.h>

char *itoa(unsigned int value, char *str, int base)
```

### DESCRIPTION

Converts an integer **_value_** to a null-terminated string using the specified **_base_** and stores the result in the array given by **_str_** parameter.

If **_base_** is **10** and **_value_** is negative, the resulting string is preceded with a minus sign (-). With any other **_base_**, **_value_** is always treated as unsigned.

**_str_** should be an array long enough to contain any possible value, as well as the terminating **EOS**.

### RETURN VALUE
A pointer to the resulting null-terminated string, same as parameter **_str_**.

### NOTES

While this function appears in many implementations of libc, it is **not** defined in ANSI-C. It is provided to facilitate portability of legacy code.

A standard compliant alternative for the most typical cases could be achieved using the **sprintf() function.

### BUGS

See GitHub Issues: <https://github.com/znac049/mega-micros-sbc3/issues>

### AUTHOR

Bob Green <bob@chippers.org.uk>

### SEE ALSO
**sprintf**, **atoi()**, **atol**
