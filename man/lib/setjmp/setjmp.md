% setjmp(3) Version 1.0 | Library Functions Manual
***

## NAME

**setjmp** — short description

### LIBRARY

libmega C library (-lmega)

### SYNOPSIS

```
#include <setjmp.h>

int setjmp(jmp_buf env);

```

### DESCRIPTION
The **setjmp()** function saves various information about the current program state and stores it in **_env_** for later use by **longjmp()**, as a way of making non local gotos.

### RETURN VALUE
**setjmp()** returns 0 after it's initial call to indicate that program execution data has been saved in
**_env_**. When it 'fake returns' after a call to **longjmp()**, it returns the value passed to **longjmp()**.


### NOTES

None.

### BUGS

See GitHub Issues: <https://github.com/znac049/mega-micros-sbc3/issues>

### AUTHOR

Bob Green <bob@chippers.org.uk>

### SEE ALSO

