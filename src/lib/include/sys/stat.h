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

#include <stddef.h>
#include <ctype.h>                  // mode_t, time_t, off_t

/*
 * File status, as returned by stat() and fstat().
 *
 * The monitor fills this in for user programs, so its layout must be
 * the same in both - change it in one place only (here).
 */

typedef unsigned int    ino_t;      // (also in dirent.h - an identical repeat is fine in C11)
typedef unsigned short  dev_t;
typedef unsigned short  nlink_t;
typedef unsigned short  uid_t;
typedef unsigned short  gid_t;
typedef unsigned int    blksize_t;
typedef unsigned int    blkcnt_t;

struct stat {
    dev_t       st_dev;         // which mount the file is on
    ino_t       st_ino;         // inode number (0 if the fs has none)
    mode_t      st_mode;        // file type and permissions
    nlink_t     st_nlink;       // number of hard links
    uid_t       st_uid;
    gid_t       st_gid;
    off_t       st_size;        // size in bytes
    time_t      st_atime;       // last access
    time_t      st_mtime;       // last modification
    time_t      st_ctime;       // last status change
    blksize_t   st_blksize;     // best size for I/O
    blkcnt_t    st_blocks;      // space used, in 512-byte units
};

// File types (the same values ext2 stores in an inode's i_mode)
#define S_IFMT      0xf000
#define S_IFSOCK    0xc000
#define S_IFLNK     0xa000
#define S_IFREG     0x8000
#define S_IFBLK     0x6000
#define S_IFDIR     0x4000
#define S_IFCHR     0x2000
#define S_IFIFO     0x1000

#define S_ISREG(m)  (((m) & S_IFMT) == S_IFREG)
#define S_ISDIR(m)  (((m) & S_IFMT) == S_IFDIR)
#define S_ISCHR(m)  (((m) & S_IFMT) == S_IFCHR)
#define S_ISBLK(m)  (((m) & S_IFMT) == S_IFBLK)
#define S_ISLNK(m)  (((m) & S_IFMT) == S_IFLNK)
#define S_ISFIFO(m) (((m) & S_IFMT) == S_IFIFO)
#define S_ISSOCK(m) (((m) & S_IFMT) == S_IFSOCK)

/*
 * Permissions. These are also in <fcntl.h>, so they're written exactly
 * the same way here (C allows an identical repeat of a #define).
 */
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

int stat(const char *pathname, struct stat *statbuf);
int lstat(const char *pathname, struct stat *statbuf);
int fstat(int fd, struct stat *statbuf);
