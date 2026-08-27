% readdir(3) Version 1.0 | Library Functions Manual
***

## NAME

**readdir** — read a directopry

### LIBRARY

libmega C library (-lmega)

### SYNOPSIS

```
#include <stdlib>

struct dirent *readdir(DIR *dirp);
```

### DESCRIPTION
 The **readdir()** function returns a pointer to a dirent structure representing the next directory entry in the directory stream pointed to by **_dirp_**. It returns **NULL** on reaching the end of the directory stream or if an error occurred.

In this implementation, the dirent structure is defined as follows:
```
struct dirent {
    ino_t           d_ino;  
    off_t           d_off;
    unsigned short  d_reclen;
    unsigned char   d_type;
    char            d_name[256];
};
```

The fields of the dirent structure are as follows:

* d_ino - The inode number of the file (if the filesystem uses inodes).
* d_off - TBD
* d_reclen - The size, in bytes, of the returned record.
* d_type - The type of the file returned. TBD
* d_name - This field contains the null terminated filename.

The data returned by **readdir()** will be overwritten by subsequent calls to **readdir()** for the  same  directory stream.

### RETURN VALUE
On  success, **readdir()** returns  a pointer to a dirent structure.

If the end of the directory stream is reached, **NULL** is returned and **_errno_** is not changed. If an error  occurs, **NULL** is returned and **_errno_** is set to indicate the error. To distinguish end of stream from an error, set **_errno_** to zero before calling **readdir()** and then check its value if **NULL** is returned.

### ERRORS
**readdir()** can fail with the following errors:

* EBADF - invalid directory stream descriptor **_dirp_**.

### NOTES

A directory stream is opened using **opendir(3)**

The order in which filenames are read by successive calls to **readdir()** depends on the filesystem  implementation; it is unlikely that the names will be sorted in any fashion.

### BUGS

See GitHub Issues: <https://github.com/znac049/mega-micros-sbc3/issues>

### AUTHOR

Bob Green <bob@chippers.org.uk>

### SEE ALSO

**getdents(2)**, **read(2)**, **closedir(3)**, **dirfd(3)**, **readdir(3)**, **rewinddir(3)**, **seekdir(3)**, **telldir(3)**
