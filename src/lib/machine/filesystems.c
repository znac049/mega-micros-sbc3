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
#include <libgen.h>
#include <machine.h>
#include <filesystems.h>
#include <bios.h>
#include <extras.h>

#if defined(BAREMETAL)

static vfile_t cwd;

static vfs_fs_t filesystems[MAX_VIRTUAL_FILESYSTEMS];

// Note files include directories
static vfile_t vfs_files[MAX_FILES];
static vmp_t mounts[MAX_MOUNTS];

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

static vmp_t *attempt_to_mount(block_device_t *dev, uint8_t subdev) {
    vmp_t *vmp = find_free_vmp();
    vmp_t *res;

    if (vmp == NULL) {
        // kprintf("NULL vmp passed in!\n");
        return NULL;
    }

    vmp->dev_driver = dev;
    vmp->subdev = subdev;
    vmp->read_only = YES;
    vmp->block_num_in_buffer = -1;


    // Try each filesystem type in turn...
    for (int fs=0; fs<MAX_VIRTUAL_FILESYSTEMS; fs++) {
        vfs_fs_t *hand = &filesystems[fs];

        if (hand->type == VFS_TYPE_FS) {
            vmp->fs_driver = hand;
            vmp->mounted = NO;

            // kprintf("...trying to mount as '%s'\n", hand->name);

            res = hand->api.fs.mount(vmp);
            if ((res != NULL) && (vmp->mounted == YES)) {
                // Success
                kprintf("%s%d: mounted as %s\n", vmp->dev_driver->name, vmp->subdev, vmp->fs_driver->name);

                // save the mountpoint name
                snprintf(vmp->name, 16, "%s%d", vmp->dev_driver->name, vmp->subdev);

                return vmp;
            }
        }
    }

    return NULL;
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

    // Anything we can mount?
    kprintf("vfs_init: what can we mount?\n");
    for (int bd=0; bd<MAX_BLOCK_DEVICES; bd++) { 
        block_device_t *dev = &block_devices[bd];

        if (dev->active == YES) {
            kprintf("Looking at blockdev '%s', subdevs=%d\n", dev->name, dev->num_sub_devices);

            // Check if we can mount anything on each subdev
            for (uint8_t subdev=0; subdev<dev->num_sub_devices; subdev++) {
                vmp_t *mp;
            
                if ((mp = attempt_to_mount(dev, subdev)) == NULL) {
                    kprintf("vfs_init: %s%d not mounted\n", dev->name, subdev);
                }
                else {
                    kprintf("vfs_init: Success: %s%d mounted as %s\n", mp->dev_driver->name, mp->subdev, mp->fs_driver->name);
                }
            }
        }
    }

    // Open stdin/out/err
    kprintf("vfs_init: open stdin/out/err...\n");
    if ((fd = bios_open("//ser1", O_RDONLY)) < 0) {
        res = fd;
    }

    if ((fd = bios_open("//ser1", O_WRONLY)) < 0) {
        res = fd;
    }

    if ((fd = bios_open("//ser2", O_WRONLY)) < 0) {
        res = fd;
    }

    kprintf("vfs_init: Calling chdir()...\n");
    if (bios_chdir("/rom0") == NOT_OK) {
        kprintf("chdir() failed|||\n");

        return NOT_OK;
    }

    return res;
}

int vfs_shutdown(void) {
    // Close any open files...

    for (int i=0; i<MAX_FILES; i++) {
        if (vfs_files[i].open == YES) {
            bios_close(i);
        }
    }


    // unomunt everything
    for (int i=0; i<MAX_MOUNTS; i++) {
        vmp_t *mp = &mounts[i];

        if (mp->mounted == YES) {
            mp->fs_driver->api.fs.unmount(mp);
            mp->mounted = NO;
            mp->dev_driver = NULL;
            mp->fs_driver = NULL;
            mp->name[0] = EOS;
            mp->subdev = 0;
        }
    }


    return OK;
}


// Bios handlers
int bios_chdir(const char *path) {
    vmp_t *mp;
    vfile_t free_dir;
    int len;

    kprintf("\nbios_chdir('%s')\n", path);

    mp = find_mount(path);
    if (mp == NULL) {
        kprintf("bios_chdir: No path handler found for '%s'\n", path);
        return NOT_OK;
    }

    free_dir.mp = mp;
    free_dir.open = NO;

    // kprintf("bios_chdir: It's on %s%d (%s)\n", mp->dev_driver->name, mp->subdev, mp->fs_driver->name);

    len = strlen(mp->dev_driver->name)+2;   // 1 for leading slash and 1 for trailing subdev number

    // kprintf("bios_chdir: opening '%s'\n", &path[len]);

    if (mp->fs_driver->api.fs.open(&free_dir, &path[len], &cwd) == NOT_OK) {
        kprintf("bios_chdir: (*open)('%s') failed, path='%s'\n", path, &path[len]);
        return NOT_OK;
    }

    // kprintf("bios_chdir: open() returned ok\n");

    memcpy(&cwd, &free_dir, sizeof(vfile_t));
    cwd.open = YES;
    cwd.mp = mp;

    kprintf("bios_chdir: success\n");
    return OK;
}

int bios_getcwd(char*buff, size_t size) {
    if (cwd.open == NO) {
        strcpy(buff, "/limbo");
        return (int)buff;
    }

    snprintf(buff, size, "/%s%d/%s", cwd.mp->dev_driver->name, cwd.mp->subdev, cwd.path);

    return (int)buff;
}

int bios_creat(const char *pathname, mode_t mode) {
    vmp_t *mp = find_mount(pathname);

    if (mp == NULL) {
        return NOT_OK;
    }

    return NOT_OK;
}

int bios_open(const char *pathname, int flags) {
    int fd = find_free_file();
    char dir_path[PATH_MAX];
    char filename[PATH_MAX];
    vfile_t *file;

    kprintf("open('%s')\n", pathname);

    if (fd == -1) {
        printf("No free file descriptors\n");
        return NOT_OK;
    }

    file = &vfs_files[fd];
    file->mp = find_mount(pathname);
    if (file->mp == NULL) {
        // No handler found
        return NOT_OK;
    }

    // kprintf("bios_open: Gotta remove '/%s' from '%s'\n", file->mp->name, pathname);

    pathname += strlen(file->mp->name)+1;
    // kprintf("bios_open: pathname adjusted to '%s'\n", pathname);

    strcpy(dir_path, dirname((char *)pathname));
    strcpy(filename, basename((char *)pathname));

    // kprintf("bios_open: going to open '%s' in directory '%s'\n", filename, dir_path);

    // invoke the filesystem specific open function
    switch (file->mp->fs_driver->type) {
        case VFS_TYPE_CHAR:
            kprintf("bios_open: opening char device - not coded yet!\n");
            break;

        case VFS_TYPE_FS:
            // kprintf("bios_open: opening file on a filesystem\n");
            if (file->mp->fs_driver->api.fs.open(file, filename, &cwd) == NOT_OK) {
                kprintf("bios_open(): failed to open '$s' in '%s'\n", filename, dir_path);
            }

            break;

        default:
            kprintf("bios_open: Bad fs type\n");
            return NOT_OK;
    }


    file->open = YES;
    file->count = file->index = 0;
    strcpy(file->path, pathname);

    return fd;
}

int bios_close(int fd) {
    if ((fd < 0) || (fd >= MAX_FILES)) {
        return -1;
    }

    return 0;
}

int bios_read(int fd, char *buff, size_t num_bytes) {
    vfile_t *file;
    int available = 0;

    if ((fd < 0) || (fd >= MAX_FILES)) {
        return -1;
    }

    file = &vfs_files[fd];
    available = file->count - file->index;

    if (available <= 0) {
        // Buffer is empty - ask the lower layer for more

        kprintf("bios_read: asking for more data\n");

        switch (file->mp->fs_driver->type) {
            case VFS_TYPE_CHAR:
                kprintf("bios_read: reading char device - not coded yet!\n");

                break;

            case VFS_TYPE_FS:
                {
                    int count = file->mp->fs_driver->api.fs.read(file, file->buffer, sizeof(file->buffer));

                    if (count == NOT_OK) {
                        kprintf("bios_read(): failed to read up to '%d' bytes\n", sizeof(file->buffer));
                        return NOT_OK;
                    }

                    file->index = 0;
                    file->count = count;
                }
                break;

            default:
                kprintf("bios_open: Bad fs type\n");
                return NOT_OK;
        }
    }

    available = file->count - file->index;

    // We have data in the buffer
    if (num_bytes > available) {
        num_bytes = available;
    }

    memcpy(buff, &file->buffer[file->index], num_bytes);
    file->index += num_bytes;

    return num_bytes;
}

size_t bios_write(int fd, const char *buff, size_t num_bytes) {
    if ((fd < 0) || (fd >= MAX_FILES)) {
        return NOT_OK;
    }

    return num_bytes;
}

int bios_opendir(const char *pathname) {
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

ssize_t bios_getdents(int fd, void *dirp, size_t count) {
    return NOT_OK;
}

#endif // BAREMETAL