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

#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <machine.h>

#define OPEN_DIR_MAX    8

static bool_t initialised = NO;

DIR dirs[OPEN_DIR_MAX];

static void init_structures(void) {
    for (int i=0; i<OPEN_DIR_MAX; i++) {
        dirs[i].open = NO;
    }

    initialised = YES;
}

static DIR *find_free_dir(void) {
    for (int i=0; i<OPEN_DIR_MAX; i++) {
        if (dirs[i].open == NO) {
            return &dirs[i];
        }
    }

    return NULL;
}

DIR *opendir(const char *name) {
    char real_path[PATH_MAX];
    int dirfd;
    DIR *dirp;

    if (initialised == NO) {
        init_structures();
    }

    dirp = find_free_dir();
    if (dirp == NULL) {
        errno = EMFILE;
        return NULL;
    }

    if (realpath(name, real_path) == NULL) {
        return NULL;
    }

#if defined(BAREMETAL)
    dirfd = vfs_opendir(real_path);
#else
    dirfd = do_trap0(BIOS_OPENDIR, (uint32_t)real_path, 0, 0);

    if (dirfd == 0) {
        return NULL;
    }
#endif

    dirp->fd = dirfd;
    dirp->open = YES;
    
    return dirp;
}