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

#if defined(BAREMETAL)

// Max number of file descriptors
#define MAX_FILES 12
#define MAX_VFS_HANDLERS 6

#define PATH_MAX 128

#define VFS_TYPE_NONE 0
#define VFS_TYPE_CHAR 1
#define VFS_TYPE_FS   2

struct vmp {
    int zob;
};

typedef struct mountpoint vmp_t;


union vfs {
    struct {
        int (*open)(vdir_t *pwd, const char *fname);
        int (*putchar)(uint8_t minor, int ch);
        int (*getchar)(uint8_t minor);
        int (*char_available)(uint8_t minor);
        int (*flush)(uint8_t minor);
    } chardev;

    struct {
        vmp_t (*mount)(void);
        int (*unmount)(vmp_t *mp);
        int (*sync)(void);
        int (*find_path)(void);
        int (*open)(vdir_t *pwd, const char *fname);
        int (*read)(vfile_t *file, size_t n_bytes);
        int (*write)(vfile_t *file, const char *buf, size_t n_bytes);
        int (*close)(vfile_t *file);
        vdir_t *(*locate)(const char *path);
    } fs;
};

typedef union vfs vfs_t;

struct vfs_handler {
    uint8_t type;
    int (*handles_path)(const char *pathname);
    char *name;
    vfs_t handler;
};

typedef struct vfs_handler vfs_handler_t;


struct vdir {
    char path[PATH_MAX];
    bool_t valid;
    vfs_handler_t *handler;
};

typedef struct vdir vdir_t;


struct vfile {
    char path[PATH_MAX];
    bool_t valid;
    vfs_handler_t *handler;
};

typedef struct vfile vfile_t;



#endif