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

#include <libgen.h>
#include <stddef.h>
#include <string.h>

static char base_str[256];

char *basename(char *path) {
    char *slash = strrchr(path, '/');

    if (path == NULL || path[0] == EOS) {
        strcpy(base_str, ".");
        return base_str;
    }

    if (slash == NULL) {
        // No slash -> return copy of path
        strcpy(base_str, path);
        return base_str;
    }

    if (path[0] == '/' && path[1] == EOS) {
        // path is "/" -> return "/"
        strcpy(base_str, "/");
        return base_str;
    }

    // Nobody likes a trailing slash!
    if (slash[1] == EOS) {
        *slash = EOS;
        slash = strrchr(path, '/');
    }

    slash++;
    if (strlen(slash) >= sizeof(base_str)) {
        return NULL;
    }
    
    strcpy(base_str, slash);

    return base_str;;
}