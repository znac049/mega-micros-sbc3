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
#include <stdlib.h>
#include <libgen.h>
#include <dirent.h>
#include <machine.h>
#include <bios.h>
#include <extras.h>
#include <errno.h>
#include <sys/stat.h>

#include "micromon.h"
#include "ptr_array.h"

static vfile_t cwd;

// Note directories are also files
static ptr_array_t *vfs_files_array;

// Filesystems table
static vfs_fs_t filesystems[MAX_VIRTUAL_FILESYSTEMS];

// Mounts table
static ptr_array_t *vfs_mounts_array;

static vfs_fs_t *find_free_fs(void) {
    for (int i=0; i<MAX_VIRTUAL_FILESYSTEMS; i++) {
        if (filesystems[i].type == VFS_TYPE_NONE) {
            return &filesystems[i];
        }
    }

    return NULL;
}

// Is the path a character device ("//ser1", ...)? Returns its driver if so.
static vfs_fs_t *find_char_device(const char *pathname) {
    for (int i=0; i<MAX_VIRTUAL_FILESYSTEMS; i++) {
        vfs_fs_t *fs = &filesystems[i];

        if ((fs->type == VFS_TYPE_CHAR) &&
            (fs->handles_path != NULL) &&
            (fs->handles_path(pathname) == YES)) {
            return fs;
        }
    }

    return NULL;
}

/*
 * The root directory, "/". It isn't a filesystem on a device: it's a
 * read-only directory whose entries are the mounted filesystems, so
 * "cd /" then "dir" lists them. It isn't in the mount table, so the code
 * that walks the table (showmount, the command search path) doesn't see it.
 */
static vfs_fs_t root_fs;
static vmp_t root_mount;

static int root_open(vfile_t *file, const char *fname, int flags, vfile_t *dir) {
    (void)dir;

    // Paths below "/" belong to the mounts, so only "/" itself gets here
    if (fname[0] != EOS) {
        errno = ENOENT;
        return NOT_OK;
    }

    if (flags & (O_WRONLY | O_CREAT | O_TRUNC | O_APPEND)) {
        errno = EISDIR;
        return NOT_OK;
    }

    file->file_type = VFS_FT_DIR;
    file->mode = S_IFDIR | 0555;
    file->size = 0;
    file->position = 0;             // the next entry getdents() returns

    return OK;
}

static int root_close(vfile_t *file) {
    (void)file;

    return OK;
}

static int root_read(vfile_t *file, char *buf, size_t n_bytes) {
    (void)file;
    (void)buf;
    (void)n_bytes;

    errno = EISDIR;
    return NOT_OK;
}

static int root_write(vfile_t *file, const char *buf, size_t n_bytes) {
    (void)file;
    (void)buf;
    (void)n_bytes;

    errno = EISDIR;
    return NOT_OK;
}

// Only rewinding, or going back to a d_off from getdents(), makes sense
static int root_seek(vfile_t *file, off_t offset, int whence) {
    if ((whence != SEEK_SET) || (offset < 0)) {
        errno = EINVAL;
        return NOT_OK;
    }

    file->position = offset;

    return offset;
}

/*
 * Entry 0 is ".", entry 1 is "..", and entry n (n >= 2) is mount table
 * slot n - 2, skipping empty slots. file->position is the next entry.
 */
static int root_getdents(vfile_t *dir, struct dirent *out, int max_ents) {
    int saved_errno = errno;        // vfs_get_mount() sets ERANGE past the end
    int n = 0;

    while ((n < max_ents) && (dir->position < 2 + MAX_MOUNTS)) {
        off_t entry = dir->position++;
        const char *name;

        if (entry < 2) {
            name = (entry == 0)?".":"..";
            out->d_ino = 1;
        }
        else {
            vmp_t *mp = vfs_get_mount(entry - 2);

            if ((mp == NULL) || (mp->mounted == NO)) {
                continue;
            }

            name = mp->name;
            out->d_ino = entry;
        }

        out->d_off = dir->position;
        out->d_reclen = sizeof(struct dirent);
        out->d_type = DT_DIR;
        strcpy(out->d_name, name);

        out++;
        n++;
    }

    errno = saved_errno;

    return n;
}

static int root_stat(vfile_t *file, struct stat *st) {
    (void)file;

    st->st_ino = 1;
    st->st_mode = S_IFDIR | 0555;
    st->st_size = 0;
    st->st_blocks = 0;

    return OK;
}

static void setup_root(void) {
    memset(&root_fs, 0, sizeof(root_fs));
    root_fs.type = VFS_TYPE_FS;
    root_fs.name = "root";
    root_fs.api.fs.open = root_open;
    root_fs.api.fs.close = root_close;
    root_fs.api.fs.read = root_read;
    root_fs.api.fs.write = root_write;
    root_fs.api.fs.seek = root_seek;
    root_fs.api.fs.getdents = root_getdents;
    root_fs.api.fs.stat = root_stat;

    memset(&root_mount, 0, sizeof(root_mount));
    root_mount.name[0] = EOS;
    root_mount.fs_driver = &root_fs;
    root_mount.dev_driver = NULL;
    root_mount.mounted = YES;
    root_mount.read_only = YES;
    root_mount.index = MAX_MOUNTS;  // its st_dev: not a mount table slot
}

static vmp_t *find_mount_by_name(const char *name, size_t len) {
    if (len == 0) {
        return NULL;
    }

    for (int i=0; i<MAX_MOUNTS; i++) {
        vmp_t *mp = (vmp_t *)get_array_item(vfs_mounts_array, i);

        if ((mp != NULL) &&
            (mp->mounted == YES) &&
            (strlen(mp->name) == len) &&
            (strncasecmp(name, mp->name, len) == 0)) {
            return mp;
        }
    }

    return NULL;
}

// Look up a mounted filesystem by name ("rom0", "cf0", ...)
vmp_t *vfs_find_mount(const char *name) {
    return find_mount_by_name(name, strlen(name));
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

    // Nothing left: it's the root directory
    if (abs_path[0] == EOS) {
        fs_path[0] = EOS;
        *mpp = &root_mount;
        return OK;
    }

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
    vmp_t *vmp = malloc(sizeof(vmp_t));
    vmp_t *res;

    if (vmp == NULL) {
        errno = ENOMEM;
        return NULL;
    }

    memset(vmp, 0, sizeof(vmp_t));

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

    free(vmp);

    return NULL;
}

vmp_t *vfs_get_mount(int i) {
    return (vmp_t *)get_array_item(vfs_mounts_array, i);
}

/*
 * Make sure fds 0, 1 and 2 are stdin, stdout and stderr. New files always
 * get the lowest free fd, so opening in order fills any gaps.
 */
static int open_console(void) {
    static const struct {
        const char *path;
        int flags;
    } console[3] = {
        { "//ser1", O_RDONLY },     // stdin
        { "//ser1", O_WRONLY },     // stdout
        { "//ser2", O_WRONLY },     // stderr
    };
    int res = OK;

    for (int i=0; i<3; i++) {
        int fd;

        if (get_array_item(vfs_files_array, i) != 0) {
            continue;
        }

        fd = bios_open(console[i].path, console[i].flags);
        if (fd != i) {
            printk("open_console: %s opened as fd %d, not %d\n", console[i].path, fd, i);
            if (fd >= 0) {
                bios_close(fd);
            }

            res = NOT_OK;
        }
    }

    return res;
}

static int init_structures(void) {
    vfs_files_array = new_ptr_array(3, MAX_FILES);
    vfs_mounts_array = new_ptr_array(1, MAX_MOUNTS);

    if ((vfs_files_array == NULL) || (vfs_mounts_array == NULL)) {
        free_ptr_array(vfs_files_array);
        free_ptr_array(vfs_mounts_array);
        vfs_files_array = vfs_mounts_array = NULL;
        return NOT_OK;
    }

    for (int i=0; i<MAX_VIRTUAL_FILESYSTEMS; i++) {
        filesystems[i].type = VFS_TYPE_NONE;
    }

    return OK;
}

int vfs_init(void) {
    int res = OK;

    cwd.open = NO;
    strcpy(cwd.path, "/");

    setup_root();

    // Allocate and initialise filesystem structures.
    if (init_structures() == NOT_OK) {
        printk("Failed to initialise filesystem structures.\n");
        return NOT_OK;
    }

    // Register a handler for the two serial ports
    if (setup_vfs_duart_handler(find_free_fs()) == NOT_OK) {
        printk("Failed to setup the duart handler");
        res = NOT_OK;
    }

    // Register a handler for ext2 filesystems
    if (setup_vfs_ext2_handler(find_free_fs())) {
        printk("Failed to setup the ext2 handler");
        res = NOT_OK;
    }

    // Register a handler for fat filesystems
    if (setup_vfs_fat_handler(find_free_fs()) == NOT_OK) {
        printk("Failed to setup the fat handler");
        res = NOT_OK;
    }

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
                int slot = add_array_item(vfs_mounts_array, (uintptr_t)mp);

                if (slot == NOT_OK) {
                    printk("No space for mount in mount table\n");
                    free(mp);
                    res = NOT_OK;
                }
                else {
                    mp->index = slot;
                    printk("%s: mounted as %s\n", mp->dev_driver->name, mp->fs_driver->name);
                }
            }
        }
    }

    // Open stdin/out/err
    if (open_console() == NOT_OK) {
        res = NOT_OK;
    }

    if (bios_chdir("/rom0") == NOT_OK) {
        printk("chdir() failed|||\n");

        res = NOT_OK;
    }

    return res;
}

int vfs_shutdown(void) {
    // Close any open files...
    vfs_close_all();

    // unomunt everything
    for (int i=0; i<MAX_MOUNTS; i++) {
        vmp_t *mp = (vmp_t *)get_array_item(vfs_mounts_array, i);

        if ((mp != NULL) && (mp->mounted == YES)) {
            mp->fs_driver->api.fs.unmount(mp);
            mp->mounted = NO;
            mp->dev_driver = NULL;
            mp->fs_driver = NULL;
            mp->name[0] = EOS;

            free_array_item(vfs_mounts_array, i);
            free(mp);
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
    new_dir.fs = mp->fs_driver;
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

    if (cwd.mp == &root_mount) {
        snprintf(buff, size, "/");
    }
    else {
        snprintf(buff, size, "/%s/%s", cwd.mp->name, cwd.path);
    }

    return (int)buff;
}

// Permissions for files made by open(O_CREAT), which has no mode argument
#define DEFAULT_CREATE_MODE     0644

/*
 * Open (and with O_CREAT, create) a file. 'mode' gives the permissions of
 * a newly created file.
 *
 * Understands O_RDONLY, O_WRONLY, O_RDWR, O_CREAT, O_TRUNC, O_APPEND and
 * O_DIRECTORY.
 */
static int do_open(const char *pathname, int flags, mode_t mode) {
    char fs_path[PATH_MAX];
    vmp_t *mp = NULL;
    vfs_fs_t *fs;
    vfile_t *file;
    int fd;

    // printk("open('%s')\n", pathname);

    // A character device, or a file on a mounted filesystem?
    fs = find_char_device(pathname);
    if (fs != NULL) {
        if (strlen(pathname) >= sizeof(fs_path)) {
            errno = ENAMETOOLONG;
            return NOT_OK;
        }

        strcpy(fs_path, pathname);
    }
    else {
        if (resolve_path(pathname, &mp, fs_path) == NOT_OK) {
            return NOT_OK;
        }

        fs = mp->fs_driver;
    }

    file = malloc(sizeof(vfile_t));
    if (file == NULL) {
        printk("Failed to allocate a vfile_t item\n");
        errno = ENOMEM;
        return NOT_OK;
    }

    fd = add_array_item(vfs_files_array, (uintptr_t)file);
    if (fd == NOT_OK) {
        free(file);
        errno = EMFILE;
        return NOT_OK;
    }

    file->mp = mp;
    file->fs = fs;
    file->minor = 0;
    file->open = NO;
    file->index = file->count = 0;
    file->position = 0;
    file->ateof = NO;
    file->file_type = VFS_FT_UNKNOWN;
    file->size = 0;
    file->mode = mode;              // only used if the file is created
    file->readable = (flags & O_RDONLY)?YES:NO;
    file->writeable = (flags & O_WRONLY)?YES:NO;
    file->append = (flags & O_APPEND)?YES:NO;

    // invoke the filesystem specific open function
    switch (fs->type) {
        case VFS_TYPE_CHAR:
            // The driver works out which device (file->minor) from the path
            if ((fs->api.chardev.open != NULL) &&
                (fs->api.chardev.open(file, fs_path) == NOT_OK)) {
                // errno has been set by the driver
                free_array_item(vfs_files_array, fd);
                free(file);
                return NOT_OK;
            }
            break;

        case VFS_TYPE_FS:
            // The fs fills in size, mode and file_type
            if (fs->api.fs.open(file, fs_path, flags, &cwd) == NOT_OK) {
                // errno has been set by the filesystem
                free_array_item(vfs_files_array, fd);
                free(file);
                return NOT_OK;
            }
            break;

        default:
            printk("bios_open: can't open on fs type %d\n", fs->type);
            free_array_item(vfs_files_array, fd);
            free(file);
            errno = ENODEV;
            return NOT_OK;
    }

    file->open = YES;
    strcpy(file->path, fs_path);

    return fd;
}

int bios_open(const char *pathname, int flags) {
    return do_open(pathname, flags, DEFAULT_CREATE_MODE);
}

// Same as open(pathname, O_CREAT | O_WRONLY | O_TRUNC), with permissions 'mode'
int bios_creat(const char *pathname, mode_t mode) {
    return do_open(pathname, O_CREAT | O_WRONLY | O_TRUNC, mode);
}

int bios_close(int fd) {
    vfile_t *file;
    int res = OK;

    if ((fd < 0) || (fd >= MAX_FILES)) {
        errno = EBADF;
        return NOT_OK;
    }

    file = (vfile_t *)get_array_item(vfs_files_array, fd);
    if (file == NULL) {
        errno = EBADF;
        return NOT_OK;
    }


    // invoke the filesystem specific close function
    switch (file->fs->type) {
        case VFS_TYPE_CHAR:
            // Make sure everything written has gone
            if ((file->writeable == YES) && (file->fs->api.chardev.flush != NULL)) {
                file->fs->api.chardev.flush(file->minor);
            }
            break;

        case VFS_TYPE_FS:
            // printk("bios_close: closing file on a filesystem\n");
            if (file->fs->api.fs.close(file) == NOT_OK) {
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
    file->fs = NULL;

    file->index = file->count = 0;
    file->position = 0;

    file->file_type = VFS_FT_UNKNOWN;

    free_array_item(vfs_files_array, fd);
    free(file);

    return res;
}

/*
 * Read from a character device: wait for the first character, then take
 * whatever else has already arrived, up to num_bytes. Characters are passed
 * through untouched.
 */
static int char_read(vfile_t *file, char *buff, size_t num_bytes) {
    vfs_fs_t *fs = file->fs;
    size_t n = 0;
    int ch;

    if (num_bytes == 0) {
        return 0;
    }

    do {
        ch = fs->api.chardev.getchar(file->minor);
        if (ch < 0) {
            errno = EIO;
            return (n > 0)?(int)n:NOT_OK;
        }

        buff[n++] = (char)ch;
    } while ((n < num_bytes) && (fs->api.chardev.char_available(file->minor) == YES));

    return (int)n;
}

// Write to a character device, sending "\r\n" for each '\n'
static size_t char_write(vfile_t *file, const char *buff, size_t num_bytes) {
    vfs_fs_t *fs = file->fs;

    for (size_t i=0; i<num_bytes; i++) {
        if (buff[i] == '\n') {
            fs->api.chardev.putchar(file->minor, '\r');
        }

        fs->api.chardev.putchar(file->minor, buff[i]);
    }

    return num_bytes;
}

int bios_read(int fd, char *buff, size_t num_bytes) {
    vfile_t *file;
    size_t available;
    int num_read = 0;

    if ((fd < 0) || (fd >= MAX_FILES)) {
        errno = EBADF;
        return NOT_OK;
    }

    file = (vfile_t *)get_array_item(vfs_files_array, fd);
    if (file == NULL) {
        errno = EBADF;
        return NOT_OK;
    }

    if (file->fs->type == VFS_TYPE_CHAR) {
        return char_read(file, buff, num_bytes);
    }

    if (file->file_type == VFS_FT_DIR) {
        // Use getdents() to read a directory
        errno = EISDIR;
        return NOT_OK;
    }

    available = file->count - file->index;

    if (file->ateof == YES) {
        // printk("bios_read: already at EOF\n");
        return 0;
    }
            
    if (available <= 0) {
        // Buffer is empty - ask the lower layer for more

        // printk("bios_read: asking for more data\n");

        switch (file->fs->type) {
            case VFS_TYPE_FS:
                {
                    num_read = file->fs->api.fs.read(file, file->buffer, BLOCK_DEVICE_BLOCK_SIZE);

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
    vfile_t *file;

    if ((fd < 0) || (fd >= MAX_FILES)) {
        errno = EBADF;
        return NOT_OK;
    }

    file = (vfile_t *)get_array_item(vfs_files_array, fd);
    if (file == NULL) {
        errno = EBADF;
        return NOT_OK;
    }

    if (file->open == NO) {
        errno = EBADF;
        return NOT_OK;
    }

    if (file->writeable == NO) {
        errno = EBADF;
        return NOT_OK;
    }

    if (num_bytes == 0) {
        return 0;
    }

    switch (file->fs->type) {
        case VFS_TYPE_CHAR:
            return char_write(file, buff, num_bytes);

        case VFS_TYPE_FS:
            if (file->file_type == VFS_FT_DIR) {
                errno = EISDIR;
                return NOT_OK;
            }

            if ((file->append == YES) &&
                (file->fs->api.fs.seek(file, 0, SEEK_END) == NOT_OK)) {
                return NOT_OK;
            }

            // Returns the number of bytes written, or NOT_OK with errno set
            return file->fs->api.fs.write(file, buff, num_bytes);

        default:
            errno = ENODEV;
            return NOT_OK;
    }
}

// Close every open file (e.g. when a program finishes, to flush its writes)
void vfs_close_all(void) {
    for (int i=0; i<MAX_FILES; i++) {
        vfile_t *file = (vfile_t *)get_array_item(vfs_files_array, i);

        if ((file == NULL) || (file->open == NO)) {
            continue;
        }

        // Leave the console alone, but not a file a program put in its place
        if ((i <= 2) && (file->fs->type == VFS_TYPE_CHAR)) {
            continue;
        }

        bios_close(i);
    }

    // Put back any of stdin/out/err the program closed
    open_console();
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

    if ((fd < 0) || (fd >= MAX_FILES)) {
        errno = EBADF;
        return NOT_OK;
    }

    file = (vfile_t *)get_array_item(vfs_files_array, fd);
    if (file == NULL) {
        errno = EBADF;
        return NOT_OK;
    }

    if ((file->fs->type != VFS_TYPE_FS) || (file->file_type != VFS_FT_DIR)) {
        errno = ENOTDIR;
        return NOT_OK;
    }

    if (file->fs->api.fs.getdents == NULL) {
        errno = ENOSYS;
        return NOT_OK;
    }

    max_ents = count / sizeof(struct dirent);
    if ((dirp == NULL) || (max_ents == 0)) {
        errno = EINVAL;
        return NOT_OK;
    }

    n = file->fs->api.fs.getdents(file, (struct dirent *)dirp, max_ents);
    if (n == NOT_OK) {
        return NOT_OK;
    }

    return n * sizeof(struct dirent);
}

off_t bios_seek(int fd, off_t offset, int whence) {
    vfile_t *file;
    off_t where;

    if ((fd < 0) || (fd >= MAX_FILES)) {
        errno = EBADF;
        return NOT_OK;
    }

    file = (vfile_t *)get_array_item(vfs_files_array, fd);
    if (file == NULL) {
        errno = EBADF;
        return NOT_OK;
    }

    switch (file->fs->type) {
    case VFS_TYPE_CHAR:
        errno = ESPIPE;
        return NOT_OK;

    case VFS_TYPE_FS:
        where = file->fs->api.fs.seek(file, offset, whence);
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

/*
 * Fill in the fields every filesystem can supply, then let the
 * filesystem add what else it knows.
 */
static void fill_stat(vfile_t *file, struct stat *st) {
    memset(st, 0, sizeof(*st));

    st->st_dev = file->mp->index;
    st->st_mode = file->mode;
    st->st_size = file->size;
    st->st_nlink = 1;
    st->st_blksize = BLOCK_DEVICE_BLOCK_SIZE;
    st->st_blocks = (file->size + 511) / 512;

    if (file->fs->api.fs.stat != NULL) {
        file->fs->api.fs.stat(file, st);
    }
}

int bios_fstat(int fd, struct stat *st) {
    vfile_t *file;

    if ((fd < 0) || (fd >= MAX_FILES)) {
        errno = EBADF;
        return NOT_OK;
    }

    if (st == NULL) {
        errno = EFAULT;
        return NOT_OK;
    }

    file = (vfile_t *)get_array_item(vfs_files_array, fd);
    if (file == NULL) {
        errno = EBADF;
        return NOT_OK;
    }

    if (file->fs->type == VFS_TYPE_CHAR) {
        memset(st, 0, sizeof(*st));
        st->st_mode = S_IFCHR | 0666;
        st->st_nlink = 1;

        return OK;
    }

    fill_stat(file, st);

    return OK;
}

/*
 * Like bios_open(), but only as far as reading the inode: no file data
 * is read and no file descriptor is used.
 */
int bios_stat(const char *pathname, struct stat *st) {
    static vfile_t file;            // ~2.7KB - keep it off the stack
    char fs_path[PATH_MAX];
    vmp_t *mp;

    if (st == NULL) {
        errno = EFAULT;
        return NOT_OK;
    }

    if (resolve_path(pathname, &mp, fs_path) == NOT_OK) {
        return NOT_OK;
    }

    if (mp->fs_driver->type != VFS_TYPE_FS) {
        errno = ENOENT;
        return NOT_OK;
    }

    memset(&file, 0, sizeof(file));
    file.mp = mp;
    file.fs = mp->fs_driver;
    file.file_type = VFS_FT_UNKNOWN;

    if (mp->fs_driver->api.fs.open(&file, fs_path, O_RDONLY, &cwd) == NOT_OK) {
        // errno set by the filesystem
        return NOT_OK;
    }

    fill_stat(&file, st);

    if (mp->fs_driver->api.fs.close != NULL) {
        mp->fs_driver->api.fs.close(&file);
    }

    return OK;
}

int bios_isatty(int fd) {
    vfile_t *file;
    int res;

    if ((fd < 0) || (fd >= MAX_FILES)) {
        errno = EBADF;
        return NOT_OK;
    }

    file = (vfile_t *)get_array_item(vfs_files_array, fd);
    if (file == NULL) {
        errno = EBADF;
        return NOT_OK;
    }

    if (file->fs->type != VFS_TYPE_CHAR) {
        errno = ENOTTY;
        return NO;
    }

    res = file->fs->api.chardev.isatty(file->minor);

    if (res != YES) {
        errno = ENOTTY;
        return NO;
    }

    return YES;
}
