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

#include <ctype.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <machine.h>
#include <errno.h>

#include "micromon.h"

void handle_cd_command(int argc, char *argv[]) {
    if (argc != 2) {
        printk("usage: cd <path>\n");
        return;
    }

    if (chdir(argv[1]) == NOT_OK) {
        printk("%s: couldn't change directory.\n", argv[1]);
        return;
    }
}

void handle_dir_command(int argc, char *argv[]) {
    const char *path = (argc > 1)?argv[1]:".";
    DIR *d;
    struct dirent *ent;

    if (argc > 2) {
        printk("usage: dir [<path>]\n");
        return;
    }

    d = opendir(path);
    if (d == NULL) {
        printk("%s: can't open directory. errno=%d\n", path, errno);
        return;
    }

    errno = 0;
    while ((ent = readdir(d)) != NULL) {
        printk("%s%s\n", ent->d_name, (ent->d_type == DT_DIR)?"/":"");
    }

    if (errno != 0) {
        printk("readdir() failed. errno=%d\n", errno);
    }

    if (closedir(d) == NOT_OK) {
        printk("closedir() failed. errno=%d\n", errno);
    }
}

void handle_pwd_command(void) {
    char pwd[PATH_MAX];

    if (getcwd(pwd, PATH_MAX) == NULL) {
        printk("Very bad karma!\n");
        return;
    }

    printk("%s\n", pwd);
}