% chdir(3) Version 1.0 | Library Functions Manual
***

## NAME

**chdir** — change working directory

### LIBRARY

libmega C library (-lmega)

### SYNOPSIS

```
#include <unistd.h>

int chdir(const char *path);
```

### DESCRIPTION
changes the current working directory of the calling process to the directory specified in **_path_**.

### RETURN VALUE
On success, zero is returned. On error, -1 is returned, and **_errno_** is set to indicate the error.

### ERRORS
**chdir()** can fail with the following errors:

* ENOENT - The directory specified in path does not exist.
* ENOTDIR - A component of path is not a directory.

### NOTES

The current working directory is the starting point for interpreting relative pathnames (those not starting with '/').

### BUGS

See GitHub Issues: <https://github.com/znac049/mega-micros-sbc3/issues>

### AUTHOR

Bob Green <bob@chippers.org.uk>

### SEE ALSO
**getcwd()**
