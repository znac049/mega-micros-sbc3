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

// File types
#define VFS_FT_UNKNOWN  0
#define VFS_FT_REG      1
#define VFS_FT_DIR      2
#define VFS_FT_SYMLINK  7

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
        int (*open)(vfile_t *pwd, const char *fname, int flags, vfile_t *cwd);
        int (*read)(vfile_t *file, char *buf, size_t n_bytes);
        int (*write)(vfile_t *file, const char *buf, size_t n_bytes);
        int (*close)(vfile_t *file);
        int (*seek)(vfile_t *file, off_t offset, int whence);
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
    off_t position;
    unsigned int readable : 1;
    unsigned int writeable : 1;
    unsigned int ateof : 1;
    uint8_t file_type;

    uint32_t mode;
    uint32_t size;

    vfile_private_t private;
};



// ext2/e2block.c
int ext2_read_block(vmp_t *mp, uint32_t block_num, uint8_t *buffer);
int ext2_read_blocks(vmp_t *mp, uint32_t block_num, int num_blocks, uint8_t *buffer);
int ext2_read_fs_block(vmp_t *mp, uint32_t block_num, uint8_t force_read);
int ext2_init_block_follower(ext2_block_follower_t *bf, vmp_t *mp, uint32_t inode_num);
void ext2_reset_block_follower(ext2_block_follower_t *bf);
uint32_t ext2_get_next_block_num(ext2_block_follower_t *bf);
void ext2_dump_block_follower(ext2_block_follower_t *bf);

// ext2/e2dir.c
int ext2_closedir(ext2_file_t *dirp);
ext2_dirent_t *ext2_readdir(ext2_file_t *dirp);
void ext2_rewinddir(ext2_file_t *dirp);
uint32_t ext2_find_item_inode_in(vmp_t *mp, uint32_t parent_inode_num, const char *item_name, bool_t is_dir);

// ext2/e2dump.c
void dump_ext2_bg(ext2_bg_t *bg, int bg_num, ext2_sb_t *sb);
void dump_ext2_inode(ext2_inode_t *in, int in_num);
void dump_ext2_sb(ext2_sb_t *sb);
void dump_ext2_fs(ext2_fs_t *fs);

// ext2/e2endian.c
void ext2_sanitize_superblock(ext2_sb_t *src_sb, ext2_sb_t *dst_sb);
void ext2_sanitize_bg(ext2_bg_t *src_bg, ext2_bg_t *dst_bg);
void ext2_sanitize_inode(ext2_inode_t *src_in, ext2_inode_t *dst_in);
void ext2_sanitize_dirent(ext2_dirent_t *src_dp, ext2_dirent_t *dst_dp);

// ext2/ext2.c
ext2_bg_t *ext2_get_bg(ext2_fs_t *fs, uint32_t blockgroup_num);
int ext2_get_inode(vmp_t *mp, uint32_t inode_num, ext2_inode_t *inode);
vmp_t *ext2_mount(vmp_t *mp);
int ext2_umount(vmp_t *mp);
int is_ext2(ext2_sb_t *sb);
bool_t ext2_has_superblock(uint32_t bg_num);
int setup_vfs_ext2_handler(vfs_fs_t *vfs);

// ext2/e2file.c
int ext2_open(vfile_t *file, const char *name, int flags, vfile_t *cwd);
int ext2_read(vfile_t *file, char *buff, size_t count);
int ext2_write(vfile_t *file, const char *buff, size_t count);
int ext2_close(vfile_t *file);
int ext2_seek(vfile_t *file, off_t offset, int whence);

// ext2/utils.c
// void printn(const char *pfx, const uint8_t *str, int len);

// ext2/e2search.c
uint32_t e2_search(vmp_t *mp, uint32_t dir_inode_num, const char *target_name, uint8_t *file_type);

#endif