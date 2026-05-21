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

#include <fcntl.h>
#include <errno.h>
#include <string.h>

extern system_io_device_t xr68681_device;

file_table_entry_t _file_table[FILE_TABLE_SIZE];

static int find_free_slot(void) {
    for (int i=0; i<FILE_TABLE_SIZE; i++) {
        if (_file_table[i].type == DEVTYPE_NOTSET) {
            return i;
        }
    }

    return -1;
}

static inline int open_dir(const char *pathname, int flags) {
    return -1;
}

static inline int open_file(const char *pathname, int flags) {
    int ft_index = find_free_slot();

    if (ft_index < 0) {
        errno = ENOMEM;
        return -1;
    }

    // Is it a special filename - ignore flags?
    if (strcasecmp(pathname, "CON:") == 0) {
        _file_table[ft_index].type = DEVTYPE_CHAR;
        _file_table[ft_index].minor = 0;
        _file_table[ft_index].device = &xr68681_device;

        return ft_index;
    }
    else if (strcasecmp(pathname, "AUX:") == 0) {
        _file_table[ft_index].type = DEVTYPE_CHAR;
        _file_table[ft_index].minor = 1;
        _file_table[ft_index].device = &xr68681_device;

        return ft_index;
    }

    // Not a special filename

    return -1;
}

int open(const char *pathname, int flags) {
    if (flags & O_DIRECTORY)
        return open_dir(pathname, flags);
    else
        return open_file(pathname, flags);
}

int creat(const char *pathname, mode_t mode) {
    return -1;
}