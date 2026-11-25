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
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <extras.h>
#include <machine.h>

/*
 * Append the path's components to out (which holds out_len bytes, and is
 * either "" or "/..." with no trailing slash), dropping empty and "."
 * components and backing up over "..". ".." at the root stays at the root.
 */
static int add_components(char *out, size_t *out_len, const char *s) {
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
            // Back up over the last component, and the slash before it
            while ((*out_len > 0) && (out[*out_len - 1] != '/')) {
                (*out_len)--;
            }

            if (*out_len > 0) {
                (*out_len)--;
            }

            out[*out_len] = EOS;
            continue;
        }

        if (*out_len + 1 + len >= PATH_MAX) {
            return NOT_OK;
        }

        out[(*out_len)++] = '/';
        memcpy(&out[*out_len], start, len);
        *out_len += len;
        out[*out_len] = EOS;
    }

    return OK;
}

/*
 * Make a path absolute and tidy it up: "/rom0//bin/./../etc/" becomes
 * "/rom0/etc", and anything that leads back to the root becomes "/".
 * It works on the text alone: there are no symbolic links to follow.
 */
char *realpath(const char *path, char *resolved_path) {
    char out[PATH_MAX];
    size_t out_len = 0;

    if (path == NULL) {
        errno = EINVAL;
        return NULL;
    }

    if (path[0] == EOS) {
        errno = ENOENT;
        return NULL;
    }

    out[0] = EOS;

    if (path[0] != '/') {
        // It's relative to the current directory
        char cwd[PATH_MAX];

        if (getcwd(cwd, PATH_MAX) == NULL) {
            errno = ENOTDIR;
            return NULL;
        }

        if (add_components(out, &out_len, cwd) == NOT_OK) {
            errno = ENAMETOOLONG;
            return NULL;
        }
    }

    if (add_components(out, &out_len, path) == NOT_OK) {
        errno = ENAMETOOLONG;
        return NULL;
    }

    // Everything cancelled out: that's the root
    if (out_len == 0) {
        strcpy(out, "/");
    }

    // Don't like this, but it's part of the function's defined behaviour
    if (resolved_path == NULL) {
        resolved_path = malloc(PATH_MAX);

        if (resolved_path == NULL) {
            errno = ENOMEM;
            return NULL;
        }
    }

    strcpy(resolved_path, out);

    return resolved_path;
}
