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


/*
 * rm, rmdir, mkdir and chmod: delete files and empty directories, make
 * directories, and set permissions.
 */

#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

#include "micromon.h"

void handle_rm_command(int argc, char *argv[]) {
    if (argc < 2) {
        printk("usage: rm <file> [<file> ...]\n");
        return;
    }

    for (int i=1; i<argc; i++) {
        if (bios_unlink(argv[i]) != OK) {
            printk("rm: %s: %s\n", argv[i], strerror(errno));
        }
    }
}

void handle_rmdir_command(int argc, char *argv[]) {
    if (argc < 2) {
        printk("usage: rmdir <directory> [<directory> ...]\n");
        return;
    }

    for (int i=1; i<argc; i++) {
        if (bios_rmdir(argv[i]) != OK) {
            printk("rmdir: %s: %s\n", argv[i], strerror(errno));
        }
    }
}

#define MKDIR_MODE  0755

/*
 * Make each directory in turn: with -p, its parents too where they're
 * missing, and one that's already there isn't an error.
 */
static int make_parents(char *path) {
    struct stat st;

    // Each '/' after the first character ends a parent
    for (char *p = path + 1; *p != EOS; p++) {
        if (*p != '/') {
            continue;
        }

        *p = EOS;
        if ((bios_stat(path, &st) != OK) && (bios_mkdir(path, MKDIR_MODE) != OK)) {
            *p = '/';
            return NOT_OK;
        }
        *p = '/';
    }

    return OK;
}

void handle_mkdir_command(int argc, char *argv[]) {
    bool_t parents = NO;
    int first = 1;

    if ((argc >= 2) && (strcmp(argv[1], "-p") == 0)) {
        parents = YES;
        first = 2;
    }

    if (argc <= first) {
        printk("usage: mkdir [-p] <directory> [<directory> ...]\n");
        return;
    }

    for (int i=first; i<argc; i++) {
        struct stat st;

        if (parents == YES) {
            if (make_parents(argv[i]) != OK) {
                printk("mkdir: %s: %s\n", argv[i], strerror(errno));
                continue;
            }

            if ((bios_stat(argv[i], &st) == OK) && S_ISDIR(st.st_mode)) {
                continue;
            }
        }

        if (bios_mkdir(argv[i], MKDIR_MODE) != OK) {
            printk("mkdir: %s: %s\n", argv[i], strerror(errno));
        }
    }
}

// The mode is octal, as for the Unix command: 755, 0644, ...
void handle_chmod_command(int argc, char *argv[]) {
    mode_t mode = 0;
    const char *p = (argc >= 2)?argv[1]:"";

    if ((argc < 3) || (*p == EOS) || (strlen(p) > 4)) {
        printk("usage: chmod <octal mode> <file> [<file> ...]\n");
        return;
    }

    for (; *p != EOS; p++) {
        if ((*p < '0') || (*p > '7')) {
            printk("chmod: '%s' isn't an octal mode\n", argv[1]);
            return;
        }

        mode = (mode << 3) | (mode_t)(*p - '0');
    }

    for (int i=2; i<argc; i++) {
        if (bios_chmod(argv[i], mode) != OK) {
            printk("chmod: %s: %s\n", argv[i], strerror(errno));
        }
    }
}
