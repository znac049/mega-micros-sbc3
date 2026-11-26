/*
MIT License

Copyright (c) 2026 Bob Green

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <string.h>
#include <errno.h>

// A message for an errno value
char *strerror(int errnum) {
    switch (errnum) {
        case 0:             return "No error";
        case EPERM:         return "Operation not permitted";
        case ENOENT:        return "No such file or directory";
        case ESRCH:         return "No such process";
        case EINTR:         return "Interrupted function call";
        case EIO:           return "Input/output error";
        case ENXIO:         return "No such device or address";
        case E2BIG:         return "Argument list too long";
        case ENOEXEC:       return "Exec format error";
        case EBADF:         return "Bad file descriptor";
        case ECHILD:        return "No child processes";
        case EAGAIN:        return "Resource temporarily unavailable";
        case ENOMEM:        return "Not enough space";
        case EACCES:        return "Permission denied";
        case EFAULT:        return "Bad address";
        case ENOTBLK:       return "Not a block device";
        case EBUSY:         return "Device or resource busy";
        case EEXIST:        return "File exists";
        case EXDEV:         return "Improper link";
        case ENODEV:        return "No such device";
        case ENOTDIR:       return "Not a directory";
        case EISDIR:        return "Is a directory";
        case EINVAL:        return "Invalid argument";
        case ENFILE:        return "Too many open files in system";
        case EMFILE:        return "Too many open files";
        case ENOTTY:        return "Inappropriate I/O control operation";
        case ETXTBSY:       return "Text file busy";
        case EFBIG:         return "File too large";
        case ENOSPC:        return "No space left on device";
        case ESPIPE:        return "Invalid seek";
        case EROFS:         return "Read-only file system";
        case EMLINK:        return "Too many links";
        case EPIPE:         return "Broken pipe";
        case EDOM:          return "Numerical argument out of domain";
        case ERANGE:        return "Result too large";
        case EDEADLK:       return "Resource deadlock avoided";
        case ENAMETOOLONG:  return "File name too long";
        case ENOLCK:        return "No locks available";
        case ENOSYS:        return "Function not implemented";
        case ENOTEMPTY:     return "Directory not empty";
        case ELOOP:         return "Too many levels of symbolic links";
        case ENOTSUP:       return "Operation not supported";
        case EGENERIC:      return "Generic error";
        default:            return "Unknown error";
    }
}
