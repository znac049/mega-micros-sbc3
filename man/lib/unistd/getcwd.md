% getcwd(3) Version 1.0 | Library Functions Manual
***

## NAME

**getcwd** — get current working directory.

### LIBRARY

libmega C library (-lmega)

### SYNOPSIS

```
#include <unistd.h>

char *getcwd(char *buf, size_t size);
```

### DESCRIPTION
 **getcwd()** copies the absolute pathname of the current working directory to the array pointed to by **_buf_**, which is of **_length_** size.

**getwd()** does not malloc(3) any memory. The **_buf_** argument should be a pointer to an array at least **PATH_MAX** bytes long. If the length of the absolute pathname of the current working directory, including the  terminating null byte, exceeds **PATH_MAX** bytes, **NULL** is returned, and **_errno_** is set to **ENAMETOOLONG**.

### RETURN VALUE


### ERRORS
**getcwd()** can fail with the following errors:

* ENAMETOOLONG - The size of the null-terminated absolute pathname string exceeds **PATH_MAX** bytes.
* ERANGE - The **_size_** argument is less than the length of the absolute pathname of the working directory, including the terminating null byte. You need to allocate a bigger array and try again.

### NOTES

None.

### BUGS

See GitHub Issues: <https://github.com/znac049/mega-micros-sbc3/issues>

### AUTHOR

Bob Green <bob@chippers.org.uk>

### SEE ALSO

**chdir(2), open(2), unlink(2)