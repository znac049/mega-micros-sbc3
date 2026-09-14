% getpid(2) Version 1.0 | Library Functions Manual
***

## NAME

**getpid** — get process identification

### LIBRARY

libmega C library (-lmega)

### SYNOPSIS

```
#include <unistd.h>

pid_t getpid(void));
```

### DESCRIPTION
Returns the process id of the running program. 

### RETURN VALUE
On success, the process id of the current process is returned.

### ERRORS
This function always succeeds.
### NOTES

As things currently stand, the returned value will either be **1** (running in the monitor or **2** (running user program). This will not change until I implement a multi-tasking kernel. )
### BUGS

See GitHub Issues: <https://github.com/znac049/mega-micros-sbc3/issues>

### AUTHOR

Bob Green <bob@chippers.org.uk>

### SEE ALSO
**tmpnam**
