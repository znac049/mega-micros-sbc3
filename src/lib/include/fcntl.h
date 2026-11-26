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

#pragma once

#include <ctype.h>

/* File flags */
#define O_RDONLY    (1<<0)
#define O_WRONLY    (1<<1)
#define O_RDWR      (O_RDONLY | O_WRONLY)
#define O_CREAT     (1<<3)
#define O_DIRECTORY (1<<4)
#define O_TMPFILE   (1<<5)
#define O_TRUNC     (1<<6)
#define O_APPEND    (1<<7)
#define O_EXCL      (1<<8)      // with O_CREAT: fail if the file exists

/* fcntl() commands */
#define F_DUPFD     0
#define F_GETFD     1
#define F_SETFD     2
#define F_GETFL     3
#define F_SETFL     4
#define F_GETLK     5
#define F_SETLK     6
#define F_SETLKW    7

#define FD_CLOEXEC  1

/* Lock types for struct flock */
#define F_RDLCK     0
#define F_WRLCK     1
#define F_UNLCK     2

struct flock {
    short l_type;       // F_RDLCK, F_WRLCK or F_UNLCK
    short l_whence;     // SEEK_SET, SEEK_CUR or SEEK_END
    off_t l_start;
    off_t l_len;        // 0 means to the end of the file
    pid_t l_pid;
};

/* File mode constants (in octal) */
#define S_IRWXU     00700
#define S_IRUSR     00400
#define S_IWUSR     00200
#define S_IXUSR     00100
#define S_IRWXG     00070
#define S_IRGRP     00040
#define S_IWGRP     00020
#define S_IXGRP     00010
#define S_IRWXO     00007
#define S_IROTH     00004
#define S_IWOTH     00002
#define S_IXOTH     00001

#define S_ISUID     004000
#define S_ISGID     002000
#define S_ISVTX     001000



int creat(const char *pathname, mode_t mode);
int open(const char *pathname, int flags, ...);
int fcntl(int fd, int cmd, ...);