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

#include <dirent.h>
#include <ext2.h>

struct ext2_dir {
    ext2_inode_t    *inode;
    uint32_t        block_index;
    uint32_t        offset;
    ext2_dirent_t   dirent;
    ext2_fs_t       *fs;
};

typedef struct ext2_dir ext2_dir_t;

int ext2_closedir(ext2_dir_t *dirp);
ext2_dir_t *ext2_opendir(ext2_fs_t *fs, const char *name);
ext2_dirent_t *ext2_readdir(ext2_dir_t *dirp);
void ext2_rewinddir(ext2_dir_t *dirp);