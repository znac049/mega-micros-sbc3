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
        kprintf("usage: cd <path>\n");
        return;
    }

    if (chdir(argv[1]) == NOT_OK) {
        kprintf("%s: couldn't change directory.\n", argv[1]);
        return;
    }
}

void handle_dir_command(int argc, char *argv[]) {
    DIR *d = opendir(".");
    struct dirent *ent;

    if (d == NULL) {
        kprintf("opendir() failed. errno=%d\n", errno);
        return;
    }

    kprintf("dir opened ok\n");

    ent = readdir(d);
    while (ent != NULL) {
        kprintf("%s\n", ent->d_name);

        ent = readdir(d);
    }

    kprintf("closing dir\n");

    if (closedir(d) == NOT_OK) {
        printf("closedir() failed. errno=%d\n", errno);
    }

    (void)argc;
    (void)argv;
}

void handle_pwd_command(void) {
    char pwd[PATH_MAX];

    if (getcwd(pwd, PATH_MAX) == NULL) {
        kprintf("Very bad karma!\n");
        return;
    }

    kprintf("%s\n", pwd);
}