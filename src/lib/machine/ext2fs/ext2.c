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
#include <extras.h>

#if defined(BAREMETAL)

static int ext2_read_superblock(vmp_t *mp, ext2_sb_t *sb) {
    // kprintf("attempt to read superblock\n");

    if (bd_read(mp->dev_driver, 0, mp->block_buffer, mp->subdev) != OK) {
        return NOT_OK;
    }

    // This only works because we have insisted on 2k block size.
    ext2_sanitize_superblock((ext2_sb_t *)(&mp->block_buffer[1024]), sb);

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
    kprintf("ext2_get_bg: bg_num=%d\n", bg_num);

    if (bg_num >= fs->num_blockgroups) {
        kprintf("ext2_get_bg: block group %d is out of range\n", bg_num);
        return NULL;
    }

    return &fs->bgdt[bg_num];
}

int ext2_get_inode(vmp_t *mp, uint32_t inode_num, ext2_inode_t *inode) {
    ext2_fs_t *fs = &mp->private.data.ext2_fs_inf;
    ext2_inode_t *ent;
    ext2_bg_t *bg;
    
    uint32_t block_group_num = (inode_num - 1) / fs->sb.s_inodes_per_group;
    uint32_t index = (inode_num - 1) % fs->sb.s_inodes_per_group;

    uint32_t block_num;
    uint32_t offset;

    kprintf("ext2_get_inode: inode %d from %s\n", inode_num, mp->name);
    kprintf("ext2_inode: the inode table we want is in bgdt %d\n", block_group_num);

    if (block_group_num >= fs->num_blockgroups) {
        kprintf("ext2_get_inode: block group %d is out of range\n", block_group_num);
        return NOT_OK;
    }

    if ((inode_num == 0) || (inode_num > fs->sb.s_inodes_count)) {
        kprintf("ext2_get_inode: inode %d is out of range\n", inode_num);
    }

    if ((bg = ext2_get_bg(fs, block_group_num)) == NULL) {
        kprintf("ext2_get_inode: Failed to get bg %d\n", block_group_num);
        return NOT_OK;
    }

    kprintf("ext2_get_inode: we retrieved bgdt %d:\n", block_group_num);
    dump_ext2_bg(bg, block_group_num, &fs->sb);


    block_num = (index * fs->sb.s_inode_size) / BLOCK_DEVICE_BLOCK_SIZE;

    offset = (index * fs->sb.s_inode_size) % BLOCK_DEVICE_BLOCK_SIZE; 

    kprintf("ext2_get_inode: inode %d is in block %d + %d + %d, offset %d\n", 
            inode_num,
            block_group_num * fs->sb.s_blocks_per_group,
            bg->bg_inode_table, 
            block_num, 
            offset);

    block_num = block_num +  (block_group_num * fs->sb.s_blocks_per_group) + bg->bg_inode_table;
    if (ext2_read_fs_block(mp, block_num, NO) != 0) {
        kprintf("ext2_get_inode: ext_read_fs_block() failed.\n");
        return NOT_OK;
    }

    ent = (ext2_inode_t *) &mp->block_buffer[offset];
    ext2_sanitize_inode(ent, inode);
    kprintf("Block Buffer:\n");
    dump_mem(mp->block_buffer, BLOCK_DEVICE_BLOCK_SIZE, YES);

    kprintf("ext2_get_inode: inode %d:\n", inode_num);
    dump_ext2_inode(ent, inode_num);

    return OK;
}

static inline vmp_t *null(int err) {
    errno = err;
    return NULL;
}

vmp_t *ext2_mount(vmp_t *mp) {
    ext2_sb_t *sb;
    int res;
    uint32_t bg1;
    uint32_t bg2;
    uint32_t block_size;
    ext2_bg_t *bgdt;
    ext2_fs_t *ext2_private_data;

    kprintf("\next2_mount: Attempting ext2 mount of %s%d\n", mp->dev_driver->name, mp->subdev);

    if (mp == NULL) {
        kprintf("NULL mp\n");
        return null(EGENERIC);
    }

    ext2_private_data = (ext2_fs_t *)&mp->private.data.ext2_fs_inf;
    sb = &ext2_private_data->sb;
    ext2_private_data->mp = mp;

    res = ext2_read_superblock(mp, sb);
    if (res != 0) {
        return null(EIO);
    }

    if (!is_ext2(sb)) {
        // kprintf("ext2_mount: Not an ext2 filesystem!\n");
        return null(EIO);
    }

    // Calculate the number of block groups two different ways and check both
    // answers are the same.
    bg1 = sb->s_blocks_count / sb->s_blocks_per_group;
    if ((bg1 * sb->s_blocks_per_group) < sb->s_blocks_count) {
        bg1++;
    }

    bg2 = sb->s_inodes_count / sb->s_inodes_per_group;
    if ((bg2 * sb->s_inodes_per_group) < sb->s_inodes_count) {
        bg2++;
    }

    if (bg1 != bg2) {
        // kprintf("Number of Block groups calculations inconsistency: %d != %d.\n", bg1, bg2);
        return null(EGENERIC);
    }

    block_size = 1024<<sb->s_log_block_size;

    // If the block size is not 2k, then fail
    if (block_size != BLOCK_DEVICE_BLOCK_SIZE) {
        kprintf("ext2_mount: Block size is %d - should be %d\n", block_size, BLOCK_DEVICE_BLOCK_SIZE);
        return null(EGENERIC);
    }

    kprintf("bg table entries=%d, entry size=%d\n", bg1, sizeof(ext2_bg_t));
    kprintf("bg table entries per block=%d\n", block_size / sizeof(ext2_bg_t));

    kprintf("ext2_mount: malloc(%d)...\n", sizeof(ext2_bg_t) * bg1);
    bgdt = malloc(sizeof(ext2_bg_t) * bg1);

    if (bgdt == NULL) {
        kprintf("ext2_mount: Out of memory.\n");
        return null(ENOMEM);
    }

    // All good !
    mp->block_num_in_buffer = 0;
    mp->block_in_buffer_valid = NO;

    ext2_private_data->num_blockgroups = bg1;
    ext2_private_data->bgdt = bgdt;

    // Read the Block Group Descriptor Table...
    // kprintf("ext2_mount: num_blockgroups=%d\n", bg1);
    // kprintf("ext2_mount: Reading bgdt, bgdt=0x%08x, num_blockgroups=%d...\n", bgdt, ext2_private_data->num_blockgroups);

    kprintf("ext2_mount: need to read %d block group descriptor tables\n", bg1);

    // Read the bgdt one at a time
    for (uint32_t i=0; i<bg1; i++) {
        uint32_t bgdt_block = (i * sb->s_blocks_per_group);

        if (ext2_has_superblock(i)) {
            bgdt_block++;
        }

        if (ext2_read_fs_block(mp, bgdt_block, NO) == NOT_OK) {
            free(bgdt);

            kprintf("ext2_mount: e2_read_blocks(, %d) failed :-(\n", bgdt_block);
            return null(EIO);
        }

        ext2_sanitize_bg((ext2_bg_t *)mp->block_buffer, &bgdt[i]);
        dump_ext2_bg(&bgdt[i], i, sb);
    }

    kprintf("ext2_mount: read and sanitized %d block group descriptor tables\n", bg1);

    mp->mounted = YES;

    return mp;
}

int ext2_umount(vmp_t *mp) {
    ext2_fs_t *ext2_private_data;

    if (mp == NULL) {
        return NOT_OK;
    }

    // kprintf("unmount ext2 filesystem on partition %d.\n", fs->part_num);

    ext2_private_data = (ext2_fs_t *)&mp->private.data.ext2_fs_inf;
    free(ext2_private_data->bgdt);

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

    vfs->api.fs.mount = ext2_mount;
    vfs->api.fs.unmount = ext2_umount;
    
    vfs->api.fs.open = ext2_open;
    vfs->api.fs.read = ext2_read;
    vfs->api.fs.write = ext2_write;
    vfs->api.fs.close = ext2_close;

    return OK;
}

#endif