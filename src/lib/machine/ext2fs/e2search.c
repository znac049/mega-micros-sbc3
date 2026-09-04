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
#include <ctype.h>
#include <machine.h>
#include <ext2.h>
#include <string.h>
#include <extras.h>

#if defined(BAREMETAL)

static int names_match(const char *target, const uint8_t *possible, uint8_t plen) {
    int tlen = strlen(target);

    kprintf("do '%s' and '%s' match? ", target, possible);

    if (tlen != plen) {
        kprintf("NO :-(\n");
        return NO;
    }

    for (int i=0; i<plen; i++) {
        if (target[i] != possible[i]) {
            kprintf("NO :-(\n");
            return NO;
        }
    }

    kprintf("YES!\n");
    return YES;
}

/*
 * Given a starting directory inode number, find the inode number of target_name
 *
 */
uint32_t e2_search(vmp_t *mp, uint32_t dir_inode_num, const char *target_name) {
    ext2_inode_t dir_inode;
    ext2_block_follower_t bf;
    ext2_dirent_t *dirent;

    kprintf("e2_search: looking for an entry called '%s' in directory at inode # %d\n", target_name, dir_inode_num); 

    if (target_name[0] == EOS) {
        return dir_inode_num;
    }

    if (ext2_get_inode(mp, dir_inode_num, &dir_inode) == NOT_OK) {
        kprintf("e2_search: couldn't read inode %d\n", dir_inode_num);
        return 0;
    }

    // Loolk through all the directory entries for the target_name
    kprintf("e2_search: scanning the directory at inode %d...\n", dir_inode_num);
    ext2_init_block_follower(&bf, mp, dir_inode_num);
    for (uint32_t block_num=ext2_get_next_block_num(&bf); block_num != 0; block_num=ext2_get_next_block_num(&bf)) {
        ext2_dirent_t ent;
        uint32_t offset = 0;

        kprintf("e2_search: Read block %d\n", block_num);

        ext2_read_fs_block(mp, block_num, NO);
        while (offset < BLOCK_DEVICE_BLOCK_SIZE) {
            dirent = (ext2_dirent_t *)&mp->block_buffer[offset];
            ext2_sanitize_dirent(dirent, &ent);

            kprintf("  name->'%s'\n", ent.name);

            if (names_match(target_name, ent.name, ent.name_len)) {
                kprintf("BINGO! in=%d, rl=%d, nl=%d\n", ent.inode, ent.rec_len, ent.name_len);
                return ent.inode;
            }

            offset += ent.rec_len;
        }
    }    

    return 0;
}

#endif