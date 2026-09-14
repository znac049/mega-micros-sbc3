% dirfd(3) Version 1.0 | Library Functions Manual
***

## NAME

**dirfd** — get directory stream file descriptor

### LIBRARY

libmega C library (-lmega)

### SYNOPSIS

```
#include <dirent.h>

int dirfd(DIR *dirp);
```

### DESCRIPTION
The function **dirfd()** returns the file descriptor associated with the directory stream **_dirp_**.

Not really sure why you'd want this as messing with the fd directly will cause all sorts of bad thimgs to happen, but the function is in the standard librarry, so here it is. You have been warned.

### RETURN VALUE
Never succeeds as this implementation doesn't work that way, so -1 is always returned, and **_errno_** is set to indicate the error.

### ERRORS
**dirfd()** can fail with the following errors:

* ENOTSUP - this implementation does not associate a file descriptor with the directory stream.

### NOTES

None.

### BUGS

See GitHub Issues: <https://github.com/znac049/mega-micros-sbc3/issues>

### AUTHOR

Bob Green <bob@chippers.org.uk>

### SEE ALSO
**open(2)**, **closedir(3)**, **opendir(3)**, **readdir(3)**, **rewinddir(3)**, **seekdir(3)**, **telldir(3)**