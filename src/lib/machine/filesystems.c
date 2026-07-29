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
#include <machine.h>
#include <filesystems.h>

vdir_t cwd;

vfile_t fs_fds[MAX_FILES];

int fs_init(void) {
    cwd.valid = NO;
    strcpy(cwd.path, "/");

    for (int i=0; i<MAX_FILES; i++) {
        fs_fds[i].valid = NO;
    }

    return 0;
}

int fs_shutdown(void) {
    // Close any open files...

    for (int i=0; i<MAX_FILES; i++) {
        if (fs_fds[i].valid == YES) {
            fs_close(i);
        }
    }

    return 0;
}

static int find_free_fd(void) {
    for (int i=0; i<MAX_FILES; i++) {
        if (fs_fds[i].valid == NO) {
            return i;
        }
    }

    return -1;
}

int fs_chdir(const char *path) {
    return 0;
}

char *fs_getcwd(char*buff, size_t size) {
    return NULL;
}

vdir_t *fs_locate(const char *path) {
    return NULL;
}

int fs_creat(const char *pathname, mode_t mode) {
    return -1;
}

int fs_open(const char *pathname, int flags) {
    int fd = find_free_fd();

    return fd;
}

int fs_close(int fd) {
    if ((fd < 0) || (fd >= MAX_FILES)) {
        return -1;
    }

    return 0;
}

int fs_read(int fd, char *buff, size_t num_bytes) {
    if ((fd < 0) || (fd >= MAX_FILES)) {
        return -1;
    }

    return 0;
}

size_t fs_write(int fd, const char *buff, size_t num_bytes) {
    if ((fd < 0) || (fd >= MAX_FILES)) {
        return -1;
    }

    return num_bytes;
}