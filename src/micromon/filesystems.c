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
#include <string.h>
#include <libgen.h>
#include <dirent.h>
#include <machine.h>
#include <bios.h>
#include <extras.h>
#include <errno.h>

#include "micromon.h"

static vfile_t cwd;

static vfs_fs_t filesystems[MAX_VIRTUAL_FILESYSTEMS];

// Note files include directories
static vfile_t vfs_files[MAX_FILES];
vmp_t mounts[MAX_MOUNTS];

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
        // printk("NULL vmp passed in!\n");
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

            // printk("...trying to mount as '%s'\n", hand->name);

            res = hand->api.fs.mount(vmp);
            if ((res != NULL) && (vmp->mounted == YES)) {
                // Success
                // printk("%s%d: mounted as %s\n", vmp->dev_driver->name, vmp->subdev, vmp->fs_driver->name);

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
    // printk("vfs_init: what can we mount?\n");
    for (int bd=0; bd<MAX_BLOCK_DEVICES; bd++) { 
        block_device_t *dev = &block_devices[bd];

        if (dev->active == YES) {
            // printk("Looking at blockdev '%s', subdevs=%d\n", dev->name, dev->num_sub_devices);

            // Check if we can mount anything on each subdev
            for (uint8_t subdev=0; subdev<dev->num_sub_devices; subdev++) {
                vmp_t *mp;
            
                if ((mp = attempt_to_mount(dev, subdev)) == NULL) {
                    printk("vfs_init: %s%d not mounted\n", dev->name, subdev);
                }
                else {
                    printk("%s%d: mounted as %s\n", mp->dev_driver->name, mp->subdev, mp->fs_driver->name);
                }
            }
        }
    }

    // Open stdin/out/err
    // printk("vfs_init: open stdin/out/err...\n");
    if ((fd = bios_open("//ser1", O_RDONLY)) < 0) {
        res = fd;
    }

    if ((fd = bios_open("//ser1", O_WRONLY)) < 0) {
        res = fd;
    }

    if ((fd = bios_open("//ser2", O_WRONLY)) < 0) {
        res = fd;
    }

    // printk("vfs_init: Calling chdir()...\n");
    if (bios_chdir("/rom0") == NOT_OK) {
        printk("chdir() failed|||\n");

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

    // printk("\nbios_chdir('%s')\n", path);

    mp = find_mount(path);
    if (mp == NULL) {
        printk("bios_chdir: No path handler found for '%s'\n", path);
        return NOT_OK;
    }

    free_dir.mp = mp;
    free_dir.open = NO;

    // printk("bios_chdir: It's on %s%d (%s)\n", mp->dev_driver->name, mp->subdev, mp->fs_driver->name);

    len = strlen(mp->dev_driver->name)+2;   // 1 for leading slash and 1 for trailing subdev number

    // printk("bios_chdir: opening '%s'\n", &path[len]);

    if (mp->fs_driver->api.fs.open(&free_dir, &path[len], O_DIRECTORY, &cwd) == NOT_OK) {
        printk("bios_chdir: (*open)('%s') failed, path='%s'\n", path, &path[len]);
        return NOT_OK;
    }

    // printk("bios_chdir: open() returned ok\n");

    memcpy(&cwd, &free_dir, sizeof(vfile_t));
    cwd.open = YES;
    cwd.mp = mp;

    // printk("bios_chdir: success\n");
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

    (void)mode;

    return NOT_OK;
}

/* 
* bios_open() understands the following flags so far
*
* O_RDWR
* O_RDONLY
* O_CREAT
* O_WRONLY
* O_APPEND
* O_DIRECTORY
*/
int bios_open(const char *pathname, int flags) {
    int fd = find_free_file();
    char dir_path[PATH_MAX];
    char filename[PATH_MAX];
    vfile_t *file;

    // printk("open('%s')\n", pathname);

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

    // printk("bios_open: Gotta remove '/%s' from '%s'\n", file->mp->name, pathname);

    pathname += strlen(file->mp->name)+1;
    // printk("bios_open: pathname adjusted to '%s'\n", pathname);

    strcpy(dir_path, dirname((char *)pathname));
    strcpy(filename, basename((char *)pathname));

    file->position = 0;

    // printk("bios_open: going to open '%s' in directory '%s'\n", filename, dir_path);

    // invoke the filesystem specific open function
    switch (file->mp->fs_driver->type) {
        case VFS_TYPE_CHAR:
            printk("bios_open: opening char device - not coded yet!\n");
            break;

        case VFS_TYPE_FS:
            // printk("bios_open: opening file on a filesystem\n");
            if (file->mp->fs_driver->api.fs.open(file, filename, flags, &cwd) == NOT_OK) {
                printk("bios_open(): failed to open '%s' in '%s'\n", filename, dir_path);
            }

            break;

        default:
            printk("bios_open: Bad fs type\n");
            return NOT_OK;
    }


    file->open = YES;
    file->count = file->index = 0;
    file->position = 0;
    file->ateof = NO;
    file->file_type = VFS_FT_UNKNOWN;
    
    strcpy(file->path, pathname);

    return fd;
}

int bios_close(int fd) {
    vfile_t *file;
    int res = OK;

    if ((fd < 0) || (fd >= MAX_FILES)) {
        return NOT_OK;
    }

    file = &vfs_files[fd];

    // invoke the filesystem specific close function
    switch (file->mp->fs_driver->type) {
        case VFS_TYPE_CHAR:
            printk("bios_close: closing char device - not coded yet!\n");
            break;

        case VFS_TYPE_FS:
            // printk("bios_close: closing file on a filesystem\n");
            if (file->mp->fs_driver->api.fs.close(file) == NOT_OK) {
                printk("bios_close(): failed to close fd %d ('%s')\n", fd, file->path);
            }

            break;

        default:
            printk("bios_close: Bad fs type\n");
            res = NOT_OK;
            break;
    }

    file->open = NO;
    file->mp = NULL;

    file->index = file->count = 0;
    file->position = 0;

    file->file_type = VFS_FT_UNKNOWN;    

    return res;
}

int bios_read(int fd, char *buff, size_t num_bytes) {
    vfile_t *file;
    size_t available;
    int num_read = 0;

    if ((fd < 0) || (fd >= MAX_FILES)) {
        return -1;
    }

    file = &vfs_files[fd];
    available = file->count - file->index;

    // printk("bios_read: available bytes = %d, file position=%d, index=%d, count=%d, size=%d\n",
    //         available,
    //         file->position, file->index, file->count, file->size);

    if (file->ateof == YES) {
        // printk("bios_read: already at EOF\n");
        return 0;
    }
            
    if (available <= 0) {
        // Buffer is empty - ask the lower layer for more

        // printk("bios_read: asking for more data\n");

        switch (file->mp->fs_driver->type) {
            case VFS_TYPE_CHAR:
                printk("bios_read: reading char device - not coded yet!\n");

                break;

            case VFS_TYPE_FS:
                {
                    num_read = file->mp->fs_driver->api.fs.read(file, file->buffer, BLOCK_DEVICE_BLOCK_SIZE);

                    if (num_read == NOT_OK) {
                        printk("bios_read(): failed to read up to '%d' bytes\n", sizeof(file->buffer));
                        return NOT_OK;
                    }

                    if (num_read == 0) {
                        // printk("bios_read: EOF detected\n");
                        file->ateof = YES;
                    }

                    file->index = 0;
                    file->count = num_read;
                }
                break;

            default:
                printk("bios_open: Bad fs type\n");
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
    file->position = num_bytes;

    // Have we read past the end of file?
    if (file->position > (off_t)file->size) {
        // We have. Adjust num_bytes to reflect that;
        int extra = file->position - file->size;

        file->position = file->size;
        file->ateof = YES;

        num_bytes = num_bytes - extra;
    }

    // printk("bios_read complete: num_read=%d, file position=%d, index=%d, count=%d, size=%d\n",
    //         num_read, file->position, file->index, file->count, file->size);

    return num_bytes;
}

size_t bios_write(int fd, const char *buff, size_t num_bytes) {
    if ((fd < 0) || (fd >= MAX_FILES)) {
        return NOT_OK;
    }

    (void)buff[0];

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
    int max_dents = count / sizeof(struct dirent);

    printk("bios_getdents: can read a max of %d dents into buffer of size %d\n", max_dents, count);

    (void)fd;
    (void)dirp;

    return NOT_OK;
}

off_t bios_seek(int fd, off_t offset, int whence) {
    vfile_t *file;
    off_t where;

    if ((fd < 0) || (fd >= MAX_FILES)) {
        errno = EBADF;
        return NOT_OK;
    }

    file = &vfs_files[fd];
    where = file->position;

    printk("\nbios_seek: file pos=%d, size=%d. offset=%d, whence=%d\n", 
            file->position, file->size, offset, whence);

    switch (file->mp->fs_driver->type) {
    case VFS_TYPE_CHAR:
        printk("bios_seek: can't seek on a char device!\n");
        errno = ESPIPE;
        return NOT_OK;

    case VFS_TYPE_FS:
        // printk("bios_open: opening file on a filesystem\n");
        if ((where = file->mp->fs_driver->api.fs.seek(file, offset, whence)) == NOT_OK) {
            printk("bios_seek(): failed to seek to offset %d (%d)\n", offset, whence);
        }

        printk("bios_seek: fs level seek returned %d\n", where);

        break;

    default:
        printk("bios_seek: Bad fs type\n");
        return NOT_OK;
    }

    printk("bios_seek: returning %d\n", where);

    return where;
}

int bios_isatty(int fd) {
    vfile_t *file;
    int res;

    if ((fd < 0) || (fd >= MAX_FILES)) {
        errno = EBADF;
        return NOT_OK;
    }

    file = &vfs_files[fd];

    if (file->mp->fs_driver->type != VFS_TYPE_CHAR) {
        errno = ENOTTY;
        return NO;
    }

    res = file->mp->fs_driver->api.chardev.isatty(fd);

    if (res != YES) {
        errno = ENOTTY;
        return NO;
    }

    return YES;
}
