% midstr(3) Version 1.0 | Library Functions Manual
***

## NAME

**midstr** — return part of a string.

### LIBRARY

libmega C library (-lmega)

### SYNOPSIS

```
#include <nonstd.h>

char *midstr(char *dest, size_t max_len, const char *s, int from, int to);
```

### DESCRIPTION

Returns a portion of a the string **_s_**, specified by the **_from_** and **_to_** parameters. The string porttion is copied into the string **_dest_**, up to a limit of **_max_len_** characters.

### RETURN VALUE

On success, a pointer to the string **_dest_** is returned, otherwise **NULL**.

### ERRORS
**midstr()** can fail with the following errors:

* ERANGE - the **_dest_** string does not have enough space.

### NOTES

None.

### BUGS

See GitHub Issues: <https://github.com/znac049/mega-micros-sbc3/issues>

### AUTHOR

Bob Green <bob@chippers.org.uk>

### SEE ALSO

