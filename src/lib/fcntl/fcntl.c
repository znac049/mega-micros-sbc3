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

#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include <stddef.h>
#include <errno.h>
#include <sys/stat.h>

/*
 * Only one program runs at a time, so a lock can always be had: the lock
 * commands succeed without doing anything, and F_GETLK reports no lock in
 * the way. There are no file descriptor or status flags to get or set.
 */
int fcntl(int fd, int cmd, ...) {
    struct stat st;
    struct flock *lock;
    va_list args;

    if (fstat(fd, &st) != 0) {
        errno = EBADF;
        return -1;
    }

    switch (cmd) {
        case F_GETFD:
        case F_SETFD:
        case F_GETFL:
        case F_SETFL:
        case F_SETLK:
        case F_SETLKW:
            return 0;

        case F_GETLK:
            va_start(args, cmd);
            lock = va_arg(args, struct flock *);
            va_end(args);

            if (lock == NULL) {
                errno = EFAULT;
                return -1;
            }

            lock->l_type = F_UNLCK;
            return 0;

        default:
            errno = EINVAL;
            return -1;
    }
}
