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
#include <string.h>
#include <machine.h>
#include <filesystems.h>

#if defined(BAREMETAL)

static vdir_t cwd;

static vfs_handler_t filesystems[MAX_VIRTUAL_FILESYSTEMS];

static vfile_t fs_fds[MAX_FILES];

static vmp_t mounts[MAX_MOUNTS];

int vfs_open(const char *pathname, int flags);


static vmp_t *find_free_vmp(void) {
    for (int i=0; i<MAX_MOUNTS; i++) {
        if (mounts[i].mounted == NO) {
            return &mounts[i];
        }
    }

    return NULL;
}

static vfs_handler_t *find_free_handler(void) {
    for (int i=0; i<MAX_VIRTUAL_FILESYSTEMS; i++) {
        if (filesystems[i].type == VFS_TYPE_NONE) {
            return &filesystems[i];
        }
    }

    return NULL;
}

int attempt_to_mount(block_device_t *dev, uint8_t subdev) {
    vmp_t *vmp = find_free_vmp();
    vmp_t *res;

    if (vmp == NULL) {
        // kprintf("NULL vmp passed in!\n");
        return NOT_OK;
    }

    // Try each filesystem type in turn...
    for (int fs=0; fs<MAX_VIRTUAL_FILESYSTEMS; fs++) {
        vfs_handler_t *hand = &filesystems[fs];

        if (hand->type == VFS_TYPE_FS) {
            vmp->dev = dev;
            vmp->subdev = subdev;

            vmp->fs_handler = hand;
            vmp->read_only = YES;
            vmp->mounted = NO;

            // kprintf("...trying to mount as '%s'\n", hand->name);

            res = hand->handler.fs.mount(vmp);

            if ((res != NULL) && (vmp->mounted == YES)) {
                // Success
                kprintf("%s%d: mounted as %s\n", vmp->dev->name, vmp->subdev, vmp->fs_handler->name);

                // save the mountpoint name
                snprintf(vmp->name, 16, "%s%d", vmp->dev->name, vmp->subdev);

                return OK;
            }
        }
    }

    return NOT_OK;
}

int vfs_init(void) {
    int res = OK;
    int fd;

    cwd.valid = NO;
    strcpy(cwd.path, "/");

    vfs_chdir("/rom0");

    // Initialise the files table
    for (int i=0; i<MAX_FILES; i++) {
        fs_fds[i].open = NO;
    }

    // Initialise the mounts table
    for (int i=0; i<MAX_MOUNTS; i++) {
        mounts[i].mounted = NO;
    }

    // Initialise the filesystems
    for (int i=0; i<MAX_VIRTUAL_FILESYSTEMS; i++) {
        filesystems[i].type = VFS_TYPE_NONE;
    }

    // Register a handler for the two serial ports
    res = setup_vfs_duart_handler(find_free_handler());

    // Register a handler for ext2 filesystems
    res = setup_vfs_ext2_handler(find_free_handler());

    // Open stdin/out/err
    if ((fd = vfs_open("//usb1", O_RDONLY)) < 0) {
        res = fd;
    }

    if ((fd = vfs_open("//usb1", O_WRONLY)) < 0) {
        res = fd;
    }

    if ((fd = vfs_open("//usb2", O_WRONLY)) < 0) {
        res = fd;
    }

    // Anything we can mount?
    for (int bd=0; bd<MAX_BLOCK_DEVICES; bd++) { 
        block_device_t *dev = &block_devices[bd];

        if (dev->active == YES) {
            // kprintf("Looking at blockdev '%s', subdevs=%d\n", dev->name, dev->num_sub_devices);

            // Check if we can mount anything on each subdev
            for (uint8_t subdev=0; subdev<dev->num_sub_devices; subdev++) {
                if (attempt_to_mount(dev, subdev) == OK) {
                    return OK;
                }
            }
        }
    }
    
    return res;
}

int vfs_shutdown(void) {
    // Close any open files...

    for (int i=0; i<MAX_FILES; i++) {
        if (fs_fds[i].open == YES) {
            vfs_close(i);
        }
    }

    return OK;
}

static int find_free_fd(void) {
    for (int i=0; i<MAX_FILES; i++) {
        if (fs_fds[i].open == NO) {
            return i;
        }
    }

    return -1;
}

static vmp_t *find_handler(const char *pathname) {
    // Find the handler responsible for the given pathname
    if (pathname[0] != '/') {
        return cwd.mp;
    }

    for (int i=0; i<MAX_MOUNTS; i++) {
        if (strncasecmp(pathname, mounts[i].name, strlen(mounts[i].name)) == 0) {
            return &mounts[i];
        }
    }

    return NULL;
}

int vfs_chdir(const char *path) {
    vmp_t *mp;
    vdir_t *target_dir;
    char *p = (char *)path;

    kprintf("chdir('%s')\n", path);

    mp = find_handler(path);
    if (mp == NULL) {
        kprintf("No path handler found!\n");
        return NOT_OK;
    }

    if (path[0] == '/') {
        // Gotta strip off the /devXX
        p = strchr(path+1, '/');
        if (p == NULL) {
            p = "/";
        }
    }

    target_dir = mp->fs_handler->handler.fs.locate(mp, p);
    if (target_dir == NULL) {
        kprintf("locate('%s') failed, p='%s'\n", path, p);
        return NOT_OK;
    }

    memcpy(&cwd, target_dir, sizeof(vdir_t));
    cwd.valid = YES;

    kprintf("chdir() success\n");
    return OK;
}

char *vfs_getcwd(char*buff, size_t size) {
    return NULL;
}

vdir_t *vfs_locate(const char *path) {
    return NULL;
}

int vfs_creat(const char *pathname, mode_t mode) {
    vmp_t *mp = find_handler(pathname);

    if (mp == NULL) {
        return NOT_OK;
    }

    return NOT_OK;
}

int vfs_open(const char *pathname, int flags) {
    int fd = find_free_fd();
    vfile_t *file;

    if (fd < 0) {
        printf("No free file descriptors\n");
        return -1;
    }

    // We have a handler
    file = &fs_fds[fd];
    file->open = YES;
    strcpy(file->path, pathname);

    // Invoke the specific filesystem handler
    file->mp = cwd.mp;
    
    return fd;
}

int vfs_close(int fd) {
    if ((fd < 0) || (fd >= MAX_FILES)) {
        return -1;
    }

    return 0;
}

int vfs_read(int fd, char *buff, size_t num_bytes) {
    if ((fd < 0) || (fd >= MAX_FILES)) {
        return -1;
    }

    return 0;
}

size_t vfs_write(int fd, const char *buff, size_t num_bytes) {
    if ((fd < 0) || (fd >= MAX_FILES)) {
        return NOT_OK;
    }

    return num_bytes;
}

#endif