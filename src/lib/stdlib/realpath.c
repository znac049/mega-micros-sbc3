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

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <nonstd.h>
#include <machine.h>

static int find_slashes(char *path, int *indexes, int max_indexes) {
    int index = 0;

    for (int i=0; path[i] != EOS; i++) {
        if (path[i] == '/') {
            indexes[index++] = i;

            if (index >= max_indexes) {
                return NOT_OK;
            }
        }
    }

    return index;
}

static int remove_bytes(char *str, int start, int len) {
    char *s = &str[start+len];
    char *d = &str[start];
    int l = strlen(str);

    if (start+len > l) {
        return NOT_OK;
    }

    while (*s) {
        *d++ = *s++;
    }

    *d = EOS;

    return OK;
}

char *realpath(const char *path, char *resolved_path) {
    int slashes[MAX_DIR_DEPTH];
    int num_slashes;
    char tmp_path[PATH_MAX*2];
    int len;

    if (path == NULL) {
        errno = EINVAL;
        return NULL;
    }

    // Don't like this, but it's part of the function's defined bahaviour
    if (resolved_path == NULL) {
        resolved_path = malloc(PATH_MAX);

        if (resolved_path == NULL) {
            errno = ENOMEM;
            return NULL;
        }
    }

    if (path[0] != '/') {
        // It's relative to the current directory
        char cwd[PATH_MAX];

        if (getcwd(cwd, PATH_MAX) == NULL) {
            errno = ENOTDIR;
            return NULL;
        }

        snprintf(tmp_path, sizeof(tmp_path), "%s/%s", cwd, path);
    }
    else {
        strncpy(tmp_path, path, sizeof(tmp_path));
    }

    num_slashes = find_slashes(tmp_path, slashes, MAX_DIR_DEPTH);
    // printf("%d slashes found at:\n", num_slashes);
    // for (int i=0; i<num_slashes; i++) {
    //     printf("%d: %d\n", i, slashes[i]);
    // }

    for (int i=0; i<num_slashes-1;) {
        int distance = slashes[i+1] - slashes[i];

        // printf("i=%d, distance=%d (%d - %d): '%s'\n", i, distance, slashes[i+1], slashes[i], tmp_path);

        // pair of adjacent slashes
        if (distance == 1) {
            remove_bytes(tmp_path, slashes[i+1], 1);
            num_slashes--;
            for (int j=i+1; j<num_slashes; j++) {
                slashes[j] = slashes[j+1]-1;
            }
        }
        else if ((distance == 2) && tmp_path[slashes[i]+1] == '.') {
            remove_bytes(tmp_path, slashes[i]+1, 2);
            num_slashes--;
            for (int j=i+1; j<num_slashes; j++) {
                slashes[j] = slashes[j+1]-2;
            }
        }
        else if ((distance == 3) && tmp_path[slashes[i]+1] == '.' && tmp_path[slashes[i]+2] == '.') {
            int num_to_remove = slashes[i+1] - slashes[i-1];

            remove_bytes(tmp_path, slashes[i-1]+1, num_to_remove);
            num_slashes = num_slashes-2;
            for (int j=i-1; j<num_slashes; j++) {
                slashes[j] = slashes[j+2]-num_to_remove;
            }
        }
        else {
            // printf("Moving on from %d\n", i);
            i++;
        }
    }

    // Trailing slash?
    len = strlen(tmp_path);
    if (tmp_path[len-1] == '/') {
        len--;
        tmp_path[len] = EOS;
        num_slashes--;
    }

    // printf("%d final slashes found at:\n", num_slashes);
    // for (int i=0; i<num_slashes; i++) {
    //     printf("%d: %d\n", i, slashes[i]);
    // }

    if (len >= PATH_MAX) {
        errno = ENAMETOOLONG;
        tmp_path[0] = EOS;
        return NULL;
    }

    strcpy(resolved_path, tmp_path);

    return resolved_path;
}
