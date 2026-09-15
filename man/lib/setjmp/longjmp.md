% longjmp(3) Version 1.0 | Library Functions Manual
***

## NAME

**longjmp** — short description

### LIBRARY

libmega C library (-lmega)

### SYNOPSIS

```
#include <setjmp.h>

__attribute__((noreturn)) void longjmp(jmp_buf env, int val);
```

### DESCRIPTION
The **longjmp()** function implements a non local goto, using the information stored in **_env_**, to
transfer execution from one location to
the location defined by an earlier call to **setjmp()**. This includes 'unwinding' the stack back to the
point at which **setjmp()** was called.

Following the call to **longjmp()**, the program continues as it the original call to **setjmp()** had returned
with the value **_val_**. If **_val_** is passed as 0, it will be modified to 1. This is because the original
call to **setjmp()** always returns 0, thus allowing the code that invoked **setjmp()** to distinguish between
the original invokation (to save the current state) and the 'fake' return (as a result of a call to **longjmp()**).

### RETURN VALUE
**longjmp()** never returns as control passes 'back' to the point at which **setjmp()** was called.


### NOTES

**longjmp()** does not check **_env_** for validity before jumping into hyperspace, so the potential exists for Bad Things(tm) to happen. Use it with care.

### BUGS

See GitHub Issues: <https://github.com/znac049/mega-micros-sbc3/issues>

### AUTHOR

Bob Green <bob@chippers.org.uk>

### SEE ALSO

