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

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <machine.h>
#include <errno.h>
#include <string.h>
#include <extras.h>

#include "micromon.h"


// sector = first sector of the partition (512 bytes)
fat_type_t fat_probe(const uint8_t *sector) {
    uint16_t bytes_per_sec = le16(&sector[11]);
    uint8_t  sec_per_clus  = sector[13];
    uint16_t reserved      = le16(&sector[14]);
    uint8_t  num_fats      = sector[16];
    uint16_t root_entries  = le16(&sector[17]);
    uint16_t tot16         = le16(&sector[19]);
    uint8_t  media         = sector[21];
    uint16_t fatsz16       = le16(&sector[22]);
    uint32_t tot32         = le32(&sector[32]);
    uint32_t fatsz32       = le32(&sector[36]);

    // Is this a FAT boot sector at all?
    if ((sector[510] != 0x55) || (sector[511] != 0xaa)) {
        return FS_NOT_FAT;
    }

    if ((sector[0] != 0xeb) && (sector[0] != 0xe9)) {
        return FS_NOT_FAT;
    }

    if ((bytes_per_sec != 512) && (bytes_per_sec != 1024) &&
        (bytes_per_sec != 2048) && (bytes_per_sec != 4096)) {
        return FS_NOT_FAT;
    }

    if ((sec_per_clus == 0) || (sec_per_clus & (sec_per_clus - 1))) {
        return FS_NOT_FAT;                      // not a power of 2
    }

    if ((reserved == 0) || (num_fats == 0) || (num_fats > 2)) {
        return FS_NOT_FAT;
    }

    if ((media != 0xf0) && (media < 0xf8)) {
        return FS_NOT_FAT;
    }

    // Count the data clusters - this alone decides the type
    uint32_t root_dir_secs = ((uint32_t)root_entries * 32 + bytes_per_sec - 1) / bytes_per_sec;
    uint32_t fat_size      = (fatsz16 != 0) ? fatsz16 : fatsz32;
    uint32_t total_secs    = (tot16 != 0) ? tot16 : tot32;
    uint32_t meta_secs     = reserved + num_fats * fat_size + root_dir_secs;

    if ((fat_size == 0) || (total_secs <= meta_secs)) {
        return FS_NOT_FAT;
    }

    uint32_t clusters = (total_secs - meta_secs) / sec_per_clus;

    if (clusters < 4085) {
        return FS_FAT12;
    }

    if (clusters < 65525) {
        return FS_FAT16;
    }

    // FAT32 must have no fixed root directory and no 16-bit FAT size
    if ((root_entries != 0) || (fatsz16 != 0)) {
        return FS_NOT_FAT;
    }

    return FS_FAT32;
}


/*
 * Open a file or directory on an fat filesystem.
 *
 * name is relative to the root of the mount and already normalised by
 * the VFS layer: no leading slash, no "." or ".." components, no
 * repeated slashes. "" means the root directory of the mount.
 */
static int fat_open(vfile_t *file, const char *name, int flags, vfile_t *cwd) {
    (void)file;
    (void)name;
    (void)flags;
    (void)cwd;

    return NOT_OK;
}


static int fat_read(vfile_t *file, char *buff, size_t count) {
    (void)file;
    (void)buff;
    (void)count;

    return NOT_OK;
}


static int fat_write(vfile_t *file, const char *buff, size_t count) {
    (void)file;
    (void)buff;
    (void)count;

    return NOT_OK;
}


static int fat_close(vfile_t *file) {
    (void)file;

    return OK;
}


static int fat_seek(vfile_t *file, off_t offset, int whence) {
    (void)file;
    (void)offset;
    (void)whence;

    return NOT_OK;
}


/*
 * Read up to max_ents directory entries from an open directory into out[].
 *
 * Directory blocks are read into the vfile's own buffer (not the shared
 * mount block buffer), using the same index/count/position bookkeeping
 * as bios_read(), so seeking (rewinddir/seekdir) works the same way:
 *
 *   position - byte offset in the directory of the next entry
 *   index    - offset of that entry within buffer[]
 *   count    - number of valid bytes in buffer[]
 *
 * Returns the number of entries stored (0 at end of directory), or
 * NOT_OK with errno set.
 */
static int fat_getdents(vfile_t *dir, struct dirent *out, int max_ents) {
    (void)dir;
    (void)out;
    (void)max_ents;

    return NOT_OK;
}


static vmp_t *fat_mount(vmp_t *mp) {
    fat_type_t ft;
    fat_fs_t *fat_private_data;

    if (bd_read(mp->dev_driver, 0, mp->block_buffer) != OK) {
        return NULL;
    }

    ft = fat_probe(mp->block_buffer);
    if (ft == FS_NOT_FAT) {
        return NULL;
    }

    fat_private_data = (fat_fs_t *)&mp->private.data.fat_fs_inf;

    fat_private_data->ft = ft;
    
    mp->mounted = YES;

    return mp;
}


static int fat_umount(vmp_t *mp) {
    (void)mp;

    return NOT_OK;
}

static int handles_path(const char *pathname) {
    if (pathname[0] == EOS) {
        return YES;
    }

    return NO;
}

int setup_vfs_fat_handler(vfs_fs_t *vfs) {
    if (vfs == NULL) {
        return NOT_OK;
    }

    vfs->type = VFS_TYPE_FS;
    vfs->handles_path = handles_path;
    vfs->name = "fat";

    vfs->api.fs.mount = fat_mount;
    vfs->api.fs.unmount = fat_umount;
    
    vfs->api.fs.open = fat_open;
    vfs->api.fs.read = fat_read;
    vfs->api.fs.write = fat_write;
    vfs->api.fs.close = fat_close;
    vfs->api.fs.seek = fat_seek;
    vfs->api.fs.getdents = fat_getdents;
    vfs->api.fs.stat = NULL;           // no inodes - the VFS defaults will do

    return OK;
}
