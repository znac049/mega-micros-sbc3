% realpath(3) Version 1.0 | Library Functions Manual
***

## NAME

**realpath** — return the canonicalized absolute pathname.

### LIBRARY

libmega C library (-lmega)

### SYNOPSIS

```
#include <stdlib.h>

char *realpath(const char *path, char *resolved_path);
```

### DESCRIPTION
**realpath()** resolves references to /./, /../ and extra '/' characters in the null-terminated string named by **_path_** to produce a canonicalized absolute pathname. The resulting  pathname is  stored  as a null-terminated string, up to a maximum of **PATH_MAX** bytes, in the buffer pointed to by **_resolved_path_**. The resulting path will have no /./ or /../ components.

If **_resolved_path_** is specified as **NULL**, then **realpath()** uses **malloc(3)** to allocate a  buffer of up to PATH_MAX bytes to hold the resolved pathname, and returns a pointer to this buffer. The caller should deallocate this buffer using free(3).

### RETURN VALUE
If there is no error, **realpath()** returns a pointer to the **_resolved_path_**.

If there is an error then **NULL* is returned and **_errno_** is set to indicate the error.

### ERRORS
**realpath()** can fail with the following errors:

* EINVAL - **)path_** is **NULL**.
* ENAMETOOLONG - A component of a pathname exceeded **NAME_MAX** characters, or an entire pathname exceeded **PATH_MAX** characters.
* ENOMEM - Out of memory.
* ENOTDIR - A component of the path prefix is not a directory.

### NOTES

None.

### BUGS

See GitHub Issues: <https://github.com/znac049/mega-micros-sbc3/issues>

### AUTHOR

Bob Green <bob@chippers.org.uk>

### SEE ALSO

**getcwd()**