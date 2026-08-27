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
#include <extras.h>

#if defined(BAREMETAL)

static vfile_t cwd;

static vfs_fs_t filesystems[MAX_VIRTUAL_FILESYSTEMS];

// Note files include directories
static vfile_t vfs_files[MAX_FILES];
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

static vfs_fs_t *find_free_fs(void) {
    for (int i=0; i<MAX_VIRTUAL_FILESYSTEMS; i++) {
        if (filesystems[i].type == VFS_TYPE_NONE) {
            return &filesystems[i];
        }
    }

    return NULL;
}

static int find_free_file(void) {
    for (int i=0; i<MAX_FILES; i++) {
        if (vfs_files[i].open == NO) {
            return i;
        }
    }

    return NOT_OK;
}

static vmp_t *find_mount(const char *pathname) {
    // Find the handler responsible for the given pathname
    if (pathname[0] != '/') {
        return cwd.mp;
    }

    for (int i=0; i<MAX_MOUNTS; i++) {
        if ((mounts[i].mounted == YES) && (strncasecmp(pathname+1, mounts[i].name, strlen(mounts[i].name)) == 0)) {
            return &mounts[i];
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
        vfs_fs_t *hand = &filesystems[fs];

        if (hand->type == VFS_TYPE_FS) {
            vmp->dev_driver = dev;
            vmp->subdev = subdev;

            vmp->fs_driver = hand;
            vmp->read_only = YES;
            vmp->mounted = NO;

            // kprintf("...trying to mount as '%s'\n", hand->name);

            res = hand->api.fs.mount(vmp);

            if ((res != NULL) && (vmp->mounted == YES)) {
                // Success
                kprintf("%s%d: mounted as %s\n", vmp->dev_driver->name, vmp->subdev, vmp->fs_driver->name);

                // save the mountpoint name
                snprintf(vmp->name, 16, "%s%d", vmp->dev_driver->name, vmp->subdev);

                return OK;
            }
        }
    }

    return NOT_OK;
}

static void init_structures(void) {
    for (int i=0; i<MAX_FILES; i++) {
        vfs_files[i].open = NO;
    }

    for (int i=0; i<MAX_MOUNTS; i++) {
        mounts[i].mounted = NO;
    }

    for (int i=0; i<MAX_VIRTUAL_FILESYSTEMS; i++) {
        filesystems[i].type = VFS_TYPE_NONE;
    }
}

int vfs_init(void) {
    int res = OK;
    int fd;

    cwd.open = NO;
    strcpy(cwd.path, "/");

    init_structures();

    // Register a handler for the two serial ports
    res = setup_vfs_duart_handler(find_free_fs());

    // Register a handler for ext2 filesystems
    res = setup_vfs_ext2_handler(find_free_fs());

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
                if (attempt_to_mount(dev, subdev) == NOT_OK) {
                    kprintf("%s%d not mounted\n", dev->name, subdev);
                }
            }
        }
    }

    kprintf("Calling chdir()...\n");
    if (vfs_chdir("/rom0") == NOT_OK) {
        kprintf("chdir() failed|||\n");

        return NOT_OK;
    }

    return res;
}

int vfs_shutdown(void) {
    // Close any open files...

    for (int i=0; i<MAX_FILES; i++) {
        if (vfs_files[i].open == YES) {
            vfs_close(i);
        }
    }

    return OK;
}

int vfs_chdir(const char *path) {
    vmp_t *mp;
    vfile_t free_dir;
    int len;

    kprintf("chdir('%s')\n", path);

    mp = find_mount(path);
    if (mp == NULL) {
        kprintf("No path handler found!\n");
        return NOT_OK;
    }
    free_dir.mp = mp;

    kprintf("It's on %s%d (%s)\n", mp->dev_driver->name, mp->subdev, mp->fs_driver->name);

    len = strlen(mp->dev_driver->name)+2;   // 1 for leading slash and 1 for trailing subdev number

    kprintf("Locating '%s'\n", &path[len]);
    if (mp->fs_driver->api.fs.opendir(&free_dir, &path[len]) == NOT_OK) {
        kprintf("opendir('%s') failed, path='%s'\n", path, &path[len]);
        return NOT_OK;
    }

    kprintf("opendir() returned ok\n");

    memcpy(&cwd, &free_dir, sizeof(vfile_t));
    cwd.open = YES;
    cwd.mp = mp;

    kprintf("chdir() success\n");
    return OK;
}

int vfs_getcwd(char*buff, size_t size) {
    if (cwd.open == NO) {
        strcpy(buff, "/limbo");
        return (int)buff;
    }

    snprintf(buff, size, "/%s%d/%s", cwd.mp->dev_driver->name, cwd.mp->subdev, cwd.path);

    return (int)buff;
}

int vfs_creat(const char *pathname, mode_t mode) {
    vmp_t *mp = find_mount(pathname);

    if (mp == NULL) {
        return NOT_OK;
    }

    return NOT_OK;
}

int vfs_open(const char *pathname, int flags) {
    int fd = find_free_file();
    vfile_t *file;

    if (fd == -1) {
        printf("No free file descriptors\n");
        return -1;
    }

    file = &vfs_files[fd];
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

int vfs_opendir(const char *pathname) {
    int fd = find_free_file();
    vfile_t *dir;

    if (fd == NOT_OK) {
        printf("No free file descriptors\n");
        return (int)NULL;
    }

    dir = &vfs_files[fd];
    dir->open = YES;
    strcpy(dir->path, pathname);

    // Invoke the specific filesystem handler
    dir->mp = find_mount(pathname);
    if (dir->mp == NULL) {
        // No handler found
        return (int)NULL;
    }

    // if (dir->mp->fs_driver->handler.fs.opendir(dir, pathname) == NULL) {
    //     printf("All gone to Hell invfs_opendir()\n");
    //     return (int)NULL;
    // }
    
    return (int)dir;
}

#endif