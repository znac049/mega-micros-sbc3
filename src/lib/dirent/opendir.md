% opendir(3) Version 1.0 | Library Functions Manual
***

## NAME

**opendir** — open a directory

### LIBRARY

libmega C library (-lmega)

### SYNOPSIS

```
#include <dirent.h>

DIR *opendir(const char *name);
```

### DESCRIPTION
 The **opendir()** function opens a directory stream corresponding to the directory name, and returns a pointer to the directory stream. The stream is positioned at the first entry in the directory.

### RETURN VALUE
 **opendir()** returns a pointer to the directory stream. On error, **NULL** is returned, and **_errno_** is set to indicate the error.

### ERRORS
**opendir()** can fail with the following errors:

* EMFILE - The limit on the number of open file descriptors has been reached.
* ENOENT - Directory does not exist, or **name** is an empty string.
* ENOTDIR - **name** is not a directory.

### NOTES

Filename entries can be read from a directory stream using readdir(3).

The underlying file descriptor of the directory stream can be obtained using **dirfd(3)**.

### BUGS

See GitHub Issues: <https://github.com/znac049/mega-micros-sbc3/issues>

### AUTHOR

Bob Green <bob@chippers.org.uk>

### SEE ALSO

**open(2)**, **closedir(3)**, **dirfd(3)**, **readdir(3)**, **rewinddir(3)**, **seekdir(3)**, **telldir(3)**