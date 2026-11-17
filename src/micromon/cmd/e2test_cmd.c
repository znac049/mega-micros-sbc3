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


#include <ctype.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <machine.h>

#include "micromon.h"

/*
 * e2test <mount>
 *
 * Exercises the ext2 allocator on a mounted filesystem without changing
 * its contents: allocates an inode and some blocks, checks the free counts
 * went down, frees them all again, and checks the counts are back where
 * they started. Run e2fsck -fn on the card afterwards - it should find
 * nothing to fix.
 */

#define TEST_BLOCKS 3

void handle_e2test_command(int argc, char *argv[]) {
    vmp_t *mp;
    ext2_fs_t *fs;
    uint32_t free_blocks, free_inodes;
    uint32_t blocks[TEST_BLOCKS];
    uint32_t inode = 0;
    int got_blocks = 0;
    bool_t ok = YES;

    if (argc != 2) {
        printk("usage: e2test <mount>   (e.g. e2test cf0)\n");
        return;
    }

    mp = vfs_find_mount(argv[1]);
    if ((mp == NULL) || (strcmp(mp->fs_driver->name, "ext2") != 0)) {
        printk("%s isn't a mounted ext2 filesystem\n", argv[1]);
        return;
    }

    fs = &mp->private.data.ext2_fs_inf;

    if (fs->writable != YES) {
        const char *why = ext2_check_writable(mp);

        printk("%s is read-only: %s\n", argv[1], (why != NULL)?why:"unknown reason");
        return;
    }

    free_blocks = fs->sb.s_free_blocks_count;
    free_inodes = fs->sb.s_free_inodes_count;

    printk("Before: %d free blocks, %d free inodes\n", free_blocks, free_inodes);

    if (ext2_alloc_inode(mp, EXT2_ROOT_INO, NO, &inode) != OK) {
        printk("  allocating an inode FAILED (errno=%d)\n", errno);
        ok = NO;
        inode = 0;
    }
    else {
        printk("  allocated inode %d\n", inode);
    }

    for (int i=0; (i<TEST_BLOCKS) && (ok == YES); i++) {
        // Ask for the blocks one after another; zero the last one
        uint32_t goal = (i == 0)?0:blocks[i-1] + 1;

        if (ext2_alloc_block(mp, goal, (i == TEST_BLOCKS-1)?YES:NO, &blocks[i]) != OK) {
            printk("  allocating block %d FAILED (errno=%d)\n", i, errno);
            ok = NO;
            break;
        }

        printk("  allocated block %d\n", blocks[i]);
        got_blocks++;
    }

    if ((ok == YES) &&
        ((fs->sb.s_free_blocks_count != free_blocks - TEST_BLOCKS) ||
         (fs->sb.s_free_inodes_count != free_inodes - 1))) {
        printk("  free counts are wrong after allocating\n");
        ok = NO;
    }

    // Give everything back, whatever happened
    for (int i=0; i<got_blocks; i++) {
        if (ext2_free_block(mp, blocks[i]) != OK) {
            printk("  freeing block %d FAILED\n", blocks[i]);
            ok = NO;
        }
    }

    if ((inode != 0) && (ext2_free_inode(mp, inode, NO) != OK)) {
        printk("  freeing inode %d FAILED\n", inode);
        ok = NO;
    }

    if (ext2_sync(mp) != OK) {
        printk("  writing the free counts FAILED\n");
        ok = NO;
    }

    printk("After:  %d free blocks, %d free inodes\n", fs->sb.s_free_blocks_count, fs->sb.s_free_inodes_count);

    if ((fs->sb.s_free_blocks_count != free_blocks) || (fs->sb.s_free_inodes_count != free_inodes)) {
        printk("  free counts didn't return to where they started\n");
        ok = NO;
    }

    printk("%s\n", (ok == YES)?"ext2 allocation test passed":"ext2 allocation test FAILED");
}
