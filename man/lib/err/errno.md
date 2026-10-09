% errno(3) Version 1.0 | Library Functions Manual
***

## NAME

**errno** — number of last error

### LIBRARY

libmega C library (-lmega)

### SYNOPSIS

```
#include <errno.h>

```

### DESCRIPTION
_errno_ is a global variable which is set by system calls and some library calls in the event of an error to indicate what went wrong.

The value of _errno_ is only meaningful when the return value of a call indicated that something went wrong. If the call was successful, then _errno_ is undefined.

Error numbers are always positive integers. The `<errno.h>` header defines symbolic names for each of it's possible values.

### NOTES

None.

### BUGS

See GitHub Issues: <https://github.com/znac049/mega-micros-sbc3/issues>

### AUTHOR

Bob Green <bob@chippers.org.uk>

### SEE ALSO
**err(3)**, **warn(3)**, **strerror(3)**
