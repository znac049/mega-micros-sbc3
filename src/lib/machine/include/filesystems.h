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
#include <blockdev.h>
#include <filesystems.h>
#include <ext2.h>

#if defined(BAREMETAL)

// Max number of file descriptors
#define MAX_FILES 12
#define MAX_VIRTUAL_FILESYSTEMS 6
#define MAX_MOUNTS 4
#define MAX_DIRS 8

#define PATH_MAX 128

#define VFS_TYPE_NONE 0
#define VFS_TYPE_CHAR 1
#define VFS_TYPE_FS   2

typedef struct vmp vmp_t;
typedef union vfs vfs_t;
typedef struct vfs_fs vfs_fs_t;
typedef struct vdir vdir_t;
typedef struct vfile vfile_t;


struct vmp {
    block_device_t *dev;
    vfs_fs_t *fs_handler;
    char name[16];

    uint8_t block_buff[BLOCK_DEVICE_BLOCK_SIZE];

    unsigned int mounted : 1;
    unsigned int read_only : 1;
    uint8_t subdev;

    // This data depends on the specific filesystem type
    union {
        ext2_fs_t   e2fs;
    } fs;
};


union vfs {
    struct {
        int (*open)(vdir_t *pwd, const char *fname);
        int (*putchar)(uint8_t minor, int ch);
        int (*getchar)(uint8_t minor);
        int (*char_available)(uint8_t minor);
        int (*flush)(uint8_t minor);
    } chardev;

    struct {
        vmp_t *(*mount)(vmp_t *mp);
        int (*unmount)(vmp_t *mp);
        int (*sync)(void);
        int (*find_path)(void);
        int (*open)(vdir_t *pwd, const char *fname);
        int (*read)(vfile_t *file, size_t n_bytes);
        int (*write)(vfile_t *file, const char *buf, size_t n_bytes);
        int (*close)(vfile_t *file);
        int (*chdir)(const char *path);
        vdir_t *(*locate)(vmp_t *mp, vdir_t *dir, const char *path);
    } fs;
};


struct vfs_fs {
    uint8_t type;
    int (*handles_path)(const char *pathname);
    char *name;
    block_device_t *dev;
    vfs_t handler;
};



struct vdir {
    char path[PATH_MAX];
    bool_t open;
    vmp_t *mp;

    union {
        ext2_dirp_t e2dir;
    } fs;
};



struct vfile {
    char path[PATH_MAX];
    bool_t open;
    vmp_t *mp;
};




#endif