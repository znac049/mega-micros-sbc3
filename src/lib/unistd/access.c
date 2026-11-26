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
#include <sys/stat.h>
#include <errno.h>

/*
 * Is the file there? There are no users or permissions to check here,
 * so R_OK and W_OK are granted for anything that exists, and X_OK for
 * anything with an execute bit (or a directory).
 */
int access(const char *pathname, int mode) {
    struct stat st;

    if (stat(pathname, &st) != 0) {
        return -1;
    }

    if ((mode & X_OK) && !S_ISDIR(st.st_mode) && !(st.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH))) {
        errno = EACCES;
        return -1;
    }

    return 0;
}
