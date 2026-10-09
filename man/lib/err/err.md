% err(3) Version 1.0 | Library Functions Manual
***

## NAME

**err** — print a formatted error message

### LIBRARY

libmega C library (-lmega)

### SYNOPSIS

```
#include <err.h>

void err(int level, const char *fmt, ...);
```

### DESCRIPTION

Display a formatted error message on standard error. If available, the last component of the program name (followed by two colons) are printed before the formatted message. A newline character always terminates the message. 

Finally, an error message obtained from **_strerror_(3)** based on the global variable _errno_ is appended to the output, prepended with a double colon, unless _fmt_ is `NULL`.

**err()** does not return, but exits with the value of the argument _level_.

### ERRORS
**err()** can fail with the following errors:

### NOTES

None.

### BUGS

See GitHub Issues: <https://github.com/znac049/mega-micros-sbc3/issues>

### AUTHOR

Bob Green <bob@chippers.org.uk>

### SEE ALSO
**printf(3)**, **strerror(3)**, **warn(3)**

