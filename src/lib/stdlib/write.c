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

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

static inline size_t write_chardev(int fd, const uint8_t *buf, size_t count) {
    for (size_t i=0; i<count; i++) {
        _file_table[fd].device->chardev.putchar(*buf++, _file_table[fd].minor);
    }

    return count;
}

size_t write(int fd, void *buf, size_t count) {
    if ((fd < 0) || (fd >= FILE_TABLE_SIZE)) {
        errno = EBADF;

        return -1;
    }

    switch (_file_table[fd].type) {
        case DEVTYPE_CHAR:
            return write_chardev(fd, (uint8_t *)buf, count);
            break;

        default:
            break;
    }

    errno = EBADF;

    return -1;
}