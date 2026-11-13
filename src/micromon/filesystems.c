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

static vmp_t *find_mount_by_name(const char *name, size_t len) {
    if (len == 0) {
        return NULL;
    }

    for (int i=0; i<MAX_MOUNTS; i++) {
        if ((mounts[i].mounted == YES) &&
            (strlen(mounts[i].name) == len) &&
            (strncasecmp(name, mounts[i].name, len) == 0)) {
            return &mounts[i];
        }
    }

    return NULL;
}

static vmp_t *find_mount(const char *pathname) {
    // Find the mount responsible for the given pathname
    const char *end;

    if (pathname[0] != '/') {
        return cwd.mp;
    }

    pathname++;
    end = strchr(pathname, '/');

    return find_mount_by_name(pathname, (end == NULL)?strlen(pathname):(size_t)(end - pathname));
}

/*
 * Nnormalise a path: drop empty and "." components,
 * resolve ".." and remove leading/trailing/repeated slashes.
 *
 *   "/rom0//bin/./../etc/"  ->  "rom0/etc"
 */
static void normalise_path(char *path) {
    char out[PATH_MAX];
    size_t out_len = 0;
    const char *s = path;

    out[0] = EOS;

    while (*s) {
        const char *start;
        size_t len;

        while (*s == '/') {
            s++;
        }

        start = s;
        while ((*s != EOS) && (*s != '/')) {
            s++;
        }

        len = s - start;

        if ((len == 0) || ((len == 1) && (start[0] == '.'))) {
            continue;
        }

        if ((len == 2) && (start[0] == '.') && (start[1] == '.')) {
            // Back up over the last component (and its separator)
            while ((out_len > 0) && (out[out_len-1] != '/')) {
                out_len--;
            }

            if (out_len > 0) {
                out_len--;
            }

            out[out_len] = EOS;
            continue;
        }

        if (out_len > 0) {
            out[out_len++] = '/';
        }

        memcpy(&out[out_len], start, len);
        out_len += len;
        out[out_len] = EOS;
    }

    strcpy(path, out);
}

/*
 * Turn a path (absolute, or relative to the current directory) into the
 * mount it lives on plus a normalised path relative to that mount's root
 * ("" for the root itself). fs_path must hold PATH_MAX bytes.
 */
static int resolve_path(const char *path, vmp_t **mpp, char *fs_path) {
    char abs_path[PATH_MAX];
    const char *rest;
    size_t mount_len;
    vmp_t *mp;

    if ((path == NULL) || (path[0] == EOS)) {
        errno = ENOENT;
        return NOT_OK;
    }

    if (path[0] == '/') {
        if (strlen(path) >= sizeof(abs_path)) {
            errno = ENAMETOOLONG;
            return NOT_OK;
        }

        strcpy(abs_path, path);
    }
    else {
        // Relative to the current directory: "/<mount>/<cwd path>/<path>"
        if (cwd.open == NO) {
            errno = ENOENT;
            return NOT_OK;
        }

        if ((strlen(cwd.mp->name) + strlen(cwd.path) + strlen(path) + 4) > sizeof(abs_path)) {
            errno = ENAMETOOLONG;
            return NOT_OK;
        }

        strcpy(abs_path, "/");
        strcat(abs_path, cwd.mp->name);
        strcat(abs_path, "/");
        strcat(abs_path, cwd.path);
        strcat(abs_path, "/");
        strcat(abs_path, path);
    }

    normalise_path(abs_path);

    // The first component names the mount, the rest is the path within it
    rest = strchr(abs_path, '/');
    mount_len = (rest == NULL)?strlen(abs_path):(size_t)(rest - abs_path);
    rest = (rest == NULL)?"":rest+1;

    mp = find_mount_by_name(abs_path, mount_len);
    if (mp == NULL) {
        errno = ENOENT;
        return NOT_OK;
    }

    strcpy(fs_path, rest);
    *mpp = mp;

    return OK;
}

static vmp_t *attempt_to_mount(block_device_t *dev) {
    vmp_t *vmp = find_free_vmp();
    vmp_t *res;

    if (vmp == NULL) {
        // printk("NULL vmp passed in!\n");
        return NULL;
    }

    vmp->dev_driver = dev;
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
                snprintf(vmp->name, 16, "%s", vmp->dev_driver->name);

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
            vmp_t *mp;

            // printk("Looking at blockdev '%s'\n", dev->name);

            if ((mp = attempt_to_mount(dev)) == NULL) {
                printk("vfs_init: %s not mounted\n", dev->name);
            }
            else {
                printk("%s: mounted as %s\n", mp->dev_driver->name, mp->fs_driver->name);
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
            // mp->subdev = 0;
        }
    }


    return OK;
}


// Bios handlers
int bios_chdir(const char *path) {
    vfile_t new_dir;
    char fs_path[PATH_MAX];
    vmp_t *mp;

    // printk("\nbios_chdir('%s')\n", path);

    if (resolve_path(path, &mp, fs_path) == NOT_OK) {
        return NOT_OK;
    }

    if (mp->fs_driver->type != VFS_TYPE_FS) {
        errno = ENOTDIR;
        return NOT_OK;
    }

    new_dir.mp = mp;
    new_dir.open = NO;
    new_dir.index = new_dir.count = 0;
    new_dir.position = 0;
    new_dir.ateof = NO;

    if (mp->fs_driver->api.fs.open(&new_dir, fs_path, O_DIRECTORY, &cwd) == NOT_OK) {
        // printk("bios_chdir: (*open)('%s') failed\n", fs_path);
        return NOT_OK;
    }

    memcpy(&cwd, &new_dir, sizeof(vfile_t));
    cwd.open = YES;
    cwd.mp = mp;
    strcpy(cwd.path, fs_path);

    return OK;
}

int bios_getcwd(char*buff, size_t size) {
    if (cwd.open == NO) {
        strcpy(buff, "/limbo");
        return (int)buff;
    }

    snprintf(buff, size, "/%s/%s", cwd.mp->dev_driver->name, cwd.path);

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
    char fs_path[PATH_MAX];
    vmp_t *mp;
    vfile_t *file;
    int fd;

    // printk("open('%s')\n", pathname);

    if (resolve_path(pathname, &mp, fs_path) == NOT_OK) {
        return NOT_OK;
    }

    fd = find_free_file();
    if (fd == NOT_OK) {
        errno = EMFILE;
        return NOT_OK;
    }

    file = &vfs_files[fd];

    file->mp = mp;
    file->open = NO;
    file->index = file->count = 0;
    file->position = 0;
    file->ateof = NO;
    file->file_type = VFS_FT_UNKNOWN;
    file->size = 0;
    file->mode = 0;
    file->readable = (flags & O_RDONLY)?YES:NO;
    file->writeable = (flags & O_WRONLY)?YES:NO;

    // invoke the filesystem specific open function
    switch (mp->fs_driver->type) {
        case VFS_TYPE_FS:
            // The fs fills in size, mode and file_type
            if (mp->fs_driver->api.fs.open(file, fs_path, flags, &cwd) == NOT_OK) {
                // errno has been set by the filesystem
                file->mp = NULL;
                return NOT_OK;
            }
            break;

        default:
            printk("bios_open: can't open on fs type %d\n", mp->fs_driver->type);
            file->mp = NULL;
            errno = ENODEV;
            return NOT_OK;
    }

    file->open = YES;
    strcpy(file->path, fs_path);

    return fd;
}

int bios_close(int fd) {
    vfile_t *file;
    int res = OK;

    if ((fd < 0) || (fd >= MAX_FILES) || (vfs_files[fd].open == NO) || (vfs_files[fd].mp == NULL)) {
        errno = EBADF;
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

    if ((fd < 0) || (fd >= MAX_FILES) || (vfs_files[fd].open == NO) || (vfs_files[fd].mp == NULL)) {
        errno = EBADF;
        return NOT_OK;
    }

    file = &vfs_files[fd];

    if (file->file_type == VFS_FT_DIR) {
        // Use getdents() to read a directory
        errno = EISDIR;
        return NOT_OK;
    }

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
    file->position += num_bytes;

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

/*
 * Read directory entries into dirp, which has room for count bytes.
 *
 * Each entry is a fixed size struct dirent. Returns the number of bytes
 * stored (a multiple of sizeof(struct dirent)), 0 at the end of the
 * directory, or NOT_OK with errno set.
 */
ssize_t bios_getdents(int fd, void *dirp, size_t count) {
    vfile_t *file;
    int max_ents;
    int n;

    if ((fd < 0) || (fd >= MAX_FILES) || (vfs_files[fd].open == NO) || (vfs_files[fd].mp == NULL)) {
        errno = EBADF;
        return NOT_OK;
    }

    file = &vfs_files[fd];

    if ((file->mp->fs_driver->type != VFS_TYPE_FS) || (file->file_type != VFS_FT_DIR)) {
        errno = ENOTDIR;
        return NOT_OK;
    }

    if (file->mp->fs_driver->api.fs.getdents == NULL) {
        errno = ENOSYS;
        return NOT_OK;
    }

    max_ents = count / sizeof(struct dirent);
    if ((dirp == NULL) || (max_ents == 0)) {
        errno = EINVAL;
        return NOT_OK;
    }

    n = file->mp->fs_driver->api.fs.getdents(file, (struct dirent *)dirp, max_ents);
    if (n == NOT_OK) {
        return NOT_OK;
    }

    return n * sizeof(struct dirent);
}

off_t bios_seek(int fd, off_t offset, int whence) {
    vfile_t *file;
    off_t where;

    if ((fd < 0) || (fd >= MAX_FILES) || (vfs_files[fd].open == NO) || (vfs_files[fd].mp == NULL)) {
        errno = EBADF;
        return NOT_OK;
    }

    file = &vfs_files[fd];

    // printk("\nbios_seek: file pos=%d, size=%d. offset=%d, whence=%d\n",
    //         file->position, file->size, offset, whence);

    switch (file->mp->fs_driver->type) {
    case VFS_TYPE_CHAR:
        errno = ESPIPE;
        return NOT_OK;

    case VFS_TYPE_FS:
        where = file->mp->fs_driver->api.fs.seek(file, offset, whence);
        if (where == NOT_OK) {
            // errno set by the filesystem
            return NOT_OK;
        }

        break;

    default:
        printk("bios_seek: Bad fs type\n");
        errno = EINVAL;
        return NOT_OK;
    }

    // Seeking back from the end means we're no longer at EOF
    if (where < (off_t)file->size) {
        file->ateof = NO;
    }

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
