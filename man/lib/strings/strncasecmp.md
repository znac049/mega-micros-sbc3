% strncasecmp(3) Version 1.0 | Library Functions Manual
***

## NAME

**strncasecmp** — compare two strings, ignoring case

### LIBRARY

libmega C library (-lmega)

### SYNOPSIS

```
#include <strings.h>

int strncasecmp(const char *s1, const char *s2, size_t n);
```

### DESCRIPTION
The  **strncasecmp()** function performs a byte-by-byte comparison of no more than the first **_n_** bytes of the strings **_s1_** and **_s2_**, ignoring the case of the characters. It returns an integer less than, equal to, or greater than zero if **_s1_** is found, respectively, to be less than, to match, or be greater than **_s2_**.

### RETURN VALUE
**strncasecmp()** returns an integer less than, equal to, or greater than zero if **_s1_** is found, respectively, to be less than, to match, or be greater than **_s2_**.

### NOTES

None.

### BUGS

See GitHub Issues: <https://github.com/znac049/mega-micros-sbc3/issues>

### AUTHOR

Bob Green <bob@chippers.org.uk>

### SEE ALSO
**strcasecmp(3)**, **strcmp(3)**, **strncmp(3)**
