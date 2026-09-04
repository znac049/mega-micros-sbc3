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
#include <ext2.h>
#include <limits.h>

#if defined(BAREMETAL)

// Max number of file descriptors
#define MAX_FILES 20
#define MAX_VIRTUAL_FILESYSTEMS 6
#define MAX_MOUNTS 4

#define VFS_TYPE_NONE 0
#define VFS_TYPE_CHAR 1
#define VFS_TYPE_FS   2

typedef struct vmp vmp_t;
typedef union vfs vfs_t;
typedef struct vfs_fs vfs_fs_t;
typedef struct vfile vfile_t;

typedef struct vmp_private vmp_private_t;
typedef struct vfile_private vfile_private_t;


struct vmp_private {
    int fs_type;

    union {
        ext2_fs_t ext2_fs_inf;
    } data;
};

struct vmp {
    char name[16];
    uint8_t block_buffer[BLOCK_DEVICE_BLOCK_SIZE];
    uint32_t block_num_in_buffer;
    uint8_t block_in_buffer_valid;

    block_device_t *dev_driver;
    vfs_fs_t *fs_driver;

    unsigned int mounted : 1;
    unsigned int read_only : 1;
    uint8_t subdev;

    // This data depends on the specific filesystem type
    vmp_private_t private;
};


union vfs {
    struct {
        int (*open)(vfile_t *pwd, const char *fname);
        int (*putchar)(uint8_t minor, int ch);
        int (*getchar)(uint8_t minor);
        int (*char_available)(uint8_t minor);
        int (*flush)(uint8_t minor);
    } chardev;

    struct {
        vmp_t *(*mount)(vmp_t *mp);
        int (*unmount)(vmp_t *mp);
        // int (*sync)(void);
        // int (*find_path)(vfile_t *dir, const char *name);
        int (*open)(vfile_t *pwd, const char *fname, vfile_t *cwd);
        int (*read)(vfile_t *file, char *buf, size_t n_bytes);
        int (*write)(vfile_t *file, const char *buf, size_t n_bytes);
        int (*close)(vfile_t *file);
    } fs;
};


struct vfs_fs {
    uint8_t type;
    int (*handles_path)(const char *pathname);
    char *name;
    block_device_t *dev;
    vfs_t api;
};


struct vfile_private {
    int fs_type;

    union {
        ext2_file_t ext2_file_inf;
    } data;
};

struct vfile {
    char path[PATH_MAX];
    bool_t open;
    vmp_t *mp;

    char buffer[BLOCK_DEVICE_BLOCK_SIZE];
    int index;
    int count;

    vfile_private_t private;
};


#endif