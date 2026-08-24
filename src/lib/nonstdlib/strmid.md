% strmid(3) Version 1.0 | Library Functions Manual
***

## NAME

**strmid** — return part of a string.

### LIBRARY

libmega C library (-lmega)

### SYNOPSIS

```
#include <nonstd.h>

char *strmid(char *dest, size_t max_len, const char *s, int from, int to);
```

### DESCRIPTION
***

Returns a portion of a the string **s**, specified by the **offset** and **length** parameters. The string porttion is copied into the string **dest**, up to a limit of **max_len** characters. If there is not enough room, **NULL** is returned.



### RETURN VALUE
***

On success, a pointer to the string **dest** is returned, otherwise **NULL**.


### NOTES
***

None.

### BUGS

See GitHub Issues: <https://github.com/znac049/mega-micros-sbc3/issues>

### AUTHOR
***

Bob Green <bob@chippers.org.uk>

### SEE ALSO
***

