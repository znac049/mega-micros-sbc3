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
#include <string.h>
#include <errno.h>
#include <extras.h>
#include <machine.h>
#include <dirent.h>

#include "micromon.h"

void dump_dirent (ext2_dirent_t *ent) {
    kprintf("\nDirectory entry:\n");
    kprintf(" inode #:    %d\n", ent->inode);
    kprintf(" record len: %d\n", ent->rec_len);
    kprintf(" name_len:   %d\n", ent->name_len);
    kprintf(" file type:  0x%02x\n", ent->file_type);
    kprintf(" name:       ");
    for (int i=0; i<ent->name_len; i++) {
        kprintf("%c", ent->name[i]);
    }
    kprintf("\n");
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
int ext2_getdents(vfile_t *dir, struct dirent *out, int max_ents) {
    int n = 0;

    if ((dir == NULL) || (dir->mp == NULL) || (out == NULL)) {
        errno = EBADF;
        return NOT_OK;
    }

    while (n < max_ents) {
        uint8_t *raw;
        uint32_t ino;
        uint16_t rec_len;
        uint8_t name_len;
        uint8_t ftype;

        if (dir->position >= (off_t)dir->size) {
            break;                                      // end of directory
        }

        if (dir->index >= dir->count) {
            // Buffer used up - fetch the next directory block
            int got = ext2_read(dir, dir->buffer, BLOCK_DEVICE_BLOCK_SIZE);

            if (got == NOT_OK) {
                errno = EIO;
                return NOT_OK;
            }

            dir->index = 0;
            dir->count = got;
        }

        /* 
         * On-disk entries are little-endian. Pick the fields out byte
         * by byte - no byte swapping or alignment worries.
         */
        raw = (uint8_t *)&dir->buffer[dir->index];
        ino = (uint32_t)raw[0] | ((uint32_t)raw[1] << 8) | ((uint32_t)raw[2] << 16) | ((uint32_t)raw[3] << 24);
        rec_len = (uint16_t)(raw[4] | (raw[5] << 8));
        name_len = raw[6];
        ftype = raw[7];         // only valid if the fs has the 'filetype' feature

        // A bad rec_len would otherwise loop forever or run off the block
        if ((rec_len < 8) || (rec_len & 3) ||
            (dir->index + rec_len > dir->count) ||
            (name_len + 8 > rec_len)) {
            kprintf("ext2_getdents: corrupt directory entry at offset %d\n", dir->position);
            errno = EIO;
            return NOT_OK;
        }

        dir->index += rec_len;
        dir->position += rec_len;

        if (ino == 0) {
            continue;                                   // unused/deleted slot
        }

        out->d_ino = ino;
        out->d_off = dir->position;                     // where the next entry starts
        out->d_reclen = sizeof(struct dirent);
        out->d_type = (ftype <= DT_LNK)?ftype:DT_UNKNOWN;
        memcpy(out->d_name, &raw[8], name_len);
        out->d_name[name_len] = EOS;

        out++;
        n++;
    }

    return n;
}
