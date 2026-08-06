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
#include <machine.h>
#include <errno.h>
#include <string.h>
#include <ext2.h>
#include <disk.h>

#if defined(BAREMETAL)

static int ext2_read_superblock(vmp_t *mp, ext2_sb_t *sb) {
    // kprintf("attempt to read superblock\n");

    if (bd_read(mp->dev, 0, mp->block_buff, mp->subdev) != OK) {
        return NOT_OK;
    }

    // This only works because we have insisted on 2k block size.
    ext2_sanitize_superblock((ext2_sb_t *)(&mp->block_buff[1024]), sb);

    return OK;
}

int is_ext2(ext2_sb_t *sb) {
    if (sb->s_magic != 0xef53) {
        return NO;
    }

    return YES;
}

static inline bool_t is_power_of(uint32_t num, uint32_t power) {
    long long p = power;

    if (num == power) {
        return YES;
    }

    while (p < num) {
        p = p * power;

        if (num == p) {
            return YES;
        }
    }

    return NO;
}

bool_t ext2_has_superblock(uint32_t bg_num) {
    if (bg_num < 2) {
        return YES;
    }

    if ((is_power_of(bg_num, 3) == YES) || (is_power_of(bg_num, 5) == YES) || (is_power_of(bg_num, 7) == YES)) {
        return YES;
    }

    return NO;
}

ext2_bg_t *ext2_get_bg(ext2_fs_t *fs, uint32_t bg_num) {
    if (bg_num >= fs->num_blockgroups) {
        printf("Block group %d is out of range\n", bg_num);
        return NULL;
    }

    return &fs->bgdt[bg_num];
}

int ext2_get_inode(ext2_fs_t *fs, uint32_t inode_num, ext2_inode_t *inode) {
    register uint32_t inodes_per_group = fs->sb.s_inodes_per_group;
    uint32_t block_group_num = (inode_num - 1) / inodes_per_group;
    uint32_t index = (inode_num - 1) % inodes_per_group;
    uint32_t block_num = (index * fs->sb.s_inode_size) / BLOCK_DEVICE_BLOCK_SIZE;
    ext2_inode_t *ent;
    ext2_bg_t *bg;
    uint32_t offset = index * fs->sb.s_inode_size;;

    if ((bg = ext2_get_bg(fs, block_group_num)) == NULL) {
        printf("Failed to get bg %d\n", block_group_num);
        return NOT_OK;
    }

    // dump_ext2_bg(bg, block_group_num, fs->sb);

    if (ext2_read_fs_block(fs, bg->bg_inode_table + block_num) != 0) {
        return NOT_OK;
    }

    ent = (ext2_inode_t *)&fs->block_buffer[offset];
    ext2_sanitize_inode(ent, inode);
    
    return OK;
}

static inline vmp_t *null(int err) {
    errno = err;
    return NULL;
}

vmp_t *ext2_mount(vmp_t *mp) {
    ext2_sb_t sb;
    int res;
    uint32_t bg1;
    uint32_t bg2;
    uint32_t block_size;
    ext2_bg_t *bgdt;
    uint32_t num_bgdt_blocks;
    ext2_fs_t *fs;

    if (mp == NULL) {
        kprintf("NULL mp\n");
        return null(EGENERIC);
    }

    fs = (ext2_fs_t *)&mp->fs;
    fs->mp = mp;

    res = ext2_read_superblock(mp, &sb);
    if (res != 0) {
        return null(EIO);
    }

    if (!is_ext2(&sb)) {
        // kprintf("Not an ext2 filesystem!\n");
        return null(EIO);
    }

    // dump_ext2_sb(&sb);

    // Calculate the number of block groups two different ways and check both
    // answers are the same.
    bg1 = sb.s_blocks_count / sb.s_blocks_per_group;
    if ((bg1 * sb.s_blocks_per_group) < sb.s_blocks_count) {
        bg1++;
    }

    bg2 = sb.s_inodes_count / sb.s_inodes_per_group;
    if ((bg2 * sb.s_inodes_per_group) < sb.s_inodes_count) {
        bg2++;
    }

    if (bg1 != bg2) {
        // printf("Number of Block groups calculations inconsistency: %d != %d.\n", bg1, bg2);
        return null(EGENERIC);
    }

    block_size = 1024<<sb.s_log_block_size;

    // If the block size is not 2k, then fail
    if (block_size != BLOCK_DEVICE_BLOCK_SIZE) {
        kprintf("Block size is %d - should be %d\n", block_size, BLOCK_DEVICE_BLOCK_SIZE);
        return null(EGENERIC);
    }

    // kprintf("bg table entries=%d, entry size=%d\n", bg1, sizeof(ext2_bg_t));
    // kprintf("bg table entries per block=%d\n", block_size / sizeof(ext2_bg_t));

    // kprintf("malloc(%d)...\n", sizeof(ext2_bg_t) * bg1);
    bgdt = malloc(sizeof(ext2_bg_t) * bg1);

    if (bgdt == NULL) {
        kprintf("Out of memory.\n");
        free(bgdt);
        return null(ENOMEM);
    }

    // All good !
    fs->num_blockgroups = bg1;
    fs->block_num_in_buffer = 0;
    fs->block_in_buffer_valid = 0;
    fs->bgdt = bgdt;

    // Read the Block Group Descriptor Table...
    num_bgdt_blocks = (fs->num_blockgroups * sizeof(ext2_bg_t)) / BLOCK_DEVICE_BLOCK_SIZE; 
    if (ext2_read_blocks(fs, 1, num_bgdt_blocks+1, (uint8_t *)bgdt) != (num_bgdt_blocks+1)) {
        free(bgdt);

        kprintf("e2_read_blocks(, 1, %d) failed :-(\n", num_bgdt_blocks+1);
        return null(EIO);
    }

    for (int i=0; i<fs->num_blockgroups; i++) {
        ext2_bg_t *bg = ext2_get_bg(fs, i);

        ext2_sanitize_bg(bg, bg);
        // dump_ext2_bg(bg, i, sb);
    }

    mp->mounted = YES;

    return mp;
}

int ext2_umount(vmp_t *mp) {
    ext2_fs_t *fs;

    if (mp == NULL) {
        return NOT_OK;
    }

    // printf("unmount ext2 filesystem on partition %d.\n", fs->part_num);

    fs = (ext2_fs_t *)&mp->fs;
    free(fs->bgdt);

    mp->mounted = NO;

    return OK;
}

static int handles_path(const char *pathname) {
    if (pathname[0] == EOS) {
        return YES;
    }

    return NO;
}

int setup_vfs_ext2_handler(vfs_fs_t *vfs) {
    if (vfs == NULL) {
        return NOT_OK;
    }

    vfs->type = VFS_TYPE_FS;
    vfs->handles_path = handles_path;
    vfs->name = "ext2";

    vfs->handler.fs.mount = ext2_mount;
    vfs->handler.fs.unmount = ext2_umount;
    
    vfs->handler.fs.sync = NULL;
    vfs->handler.fs.find_path = NULL;
    vfs->handler.fs.open = NULL;
    vfs->handler.fs.read = NULL;
    vfs->handler.fs.write = NULL;
    vfs->handler.fs.close = NULL;
    vfs->handler.fs.locate = ext2_locate;

    return OK;
}

#endif