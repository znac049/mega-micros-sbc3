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
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>

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

/*
 * dir [-a] [-b] [<path> ...]
 *
 * Lists each directory (or file) given, or the current directory, in the
 * style of "ls -l": type and permissions, links, size, modification time
 * and name, sorted by name. -b gives just the names, as before, and -a
 * includes "." and "..".
 */

#define DIR_ALL     0x01        // -a
#define DIR_BRIEF   0x02        // -b

typedef struct {
    char *name;
    struct stat st;
    bool_t have_stat;
} dir_entry_t;

static int compare_entries(const void *a, const void *b) {
    return strcmp(((const dir_entry_t *)a)->name, ((const dir_entry_t *)b)->name);
}

// "drwxr-xr-x" and so on
static void mode_string(mode_t mode, char *out) {
    static const char rwx[] = "rwxrwxrwx";
    char type;

    switch (mode & S_IFMT) {
        case S_IFDIR:   type = 'd'; break;
        case S_IFLNK:   type = 'l'; break;
        case S_IFCHR:   type = 'c'; break;
        case S_IFBLK:   type = 'b'; break;
        case S_IFIFO:   type = 'p'; break;
        case S_IFSOCK:  type = 's'; break;
        default:        type = '-'; break;
    }

    out[0] = type;
    for (int i=0; i<9; i++) {
        out[i + 1] = (mode & (0400 >> i))?rwx[i]:'-';
    }

    // setuid, setgid and sticky show in the execute positions
    if (mode & S_ISUID) {
        out[3] = (mode & S_IXUSR)?'s':'S';
    }
    if (mode & S_ISGID) {
        out[6] = (mode & S_IXGRP)?'s':'S';
    }
    if (mode & S_ISVTX) {
        out[9] = (mode & S_IXOTH)?'t':'T';
    }

    out[10] = EOS;
}

static void print_entry(const dir_entry_t *e, int flags) {
    char mode[11];
    char when[20];
    const char *suffix = "";

    if (e->have_stat && S_ISDIR(e->st.st_mode)) {
        suffix = "/";
    }

    if (flags & DIR_BRIEF) {
        printk("%s%s\n", e->name, suffix);
        return;
    }

    if (!e->have_stat) {
        printk("?????????? %3s %9s %16s  %s\n", "?", "?", "", e->name);
        return;
    }

    mode_string(e->st.st_mode, mode);

    // A time of 0 means it was never set (no RTC when it was written)
    if (e->st.st_mtime != 0) {
        struct tm tm;

        strftime(when, sizeof(when), "%Y-%m-%d %H:%M", gmtime_r(&e->st.st_mtime, &tm));
    }
    else {
        strcpy(when, "-");
    }

    if (S_ISCHR(e->st.st_mode) || S_ISBLK(e->st.st_mode)) {
        printk("%s %3d %9s %16s  %s\n", mode, (int)e->st.st_nlink, "", when, e->name);
    }
    else {
        printk("%s %3d %9ld %16s  %s%s\n", mode, (int)e->st.st_nlink, (long)e->st.st_size, when, e->name, suffix);
    }
}

// Join a directory and a name into a path
static void join_path(char *out, size_t size, const char *dir, const char *name) {
    size_t len = strlen(dir);

    if ((len > 0) && (dir[len - 1] == '/')) {
        snprintf(out, size, "%s%s", dir, name);
    }
    else {
        snprintf(out, size, "%s/%s", dir, name);
    }
}

static void list_directory(const char *path, int flags) {
    DIR *d;
    struct dirent *ent;
    dir_entry_t *entries = NULL;
    int count = 0;
    int room = 0;
    long total = 0;
    int ndirs = 0;
    int nfiles = 0;

    d = opendir(path);
    if (d == NULL) {
        printk("%s: %s\n", path, strerror(errno));
        return;
    }

    // Read them all first, so they can be sorted
    errno = 0;
    while ((ent = readdir(d)) != NULL) {
        dir_entry_t *e;

        if (!(flags & DIR_ALL) && ((strcmp(ent->d_name, ".") == 0) || (strcmp(ent->d_name, "..") == 0))) {
            continue;
        }

        if (count == room) {
            int new_room = (room == 0)?32:(room * 2);
            dir_entry_t *bigger = realloc(entries, new_room * sizeof(dir_entry_t));

            if (bigger == NULL) {
                printk("%s: out of memory after %d entries\n", path, count);
                break;
            }

            entries = bigger;
            room = new_room;
        }

        e = &entries[count];
        e->name = strdup(ent->d_name);
        if (e->name == NULL) {
            printk("%s: out of memory after %d entries\n", path, count);
            break;
        }

        e->have_stat = NO;
        if (!(flags & DIR_BRIEF) || (ent->d_type == DT_UNKNOWN)) {
            char full[PATH_MAX];

            join_path(full, sizeof(full), path, ent->d_name);
            e->have_stat = (stat(full, &e->st) == 0)?YES:NO;
        }
        else if (ent->d_type == DT_DIR) {
            // For -b, the entry's type is enough to mark directories
            e->st.st_mode = S_IFDIR;
            e->have_stat = YES;
        }

        count++;
        errno = 0;
    }

    if (errno != 0) {
        printk("%s: %s\n", path, strerror(errno));
    }

    closedir(d);

    qsort(entries, count, sizeof(dir_entry_t), compare_entries);

    for (int i=0; i<count; i++) {
        print_entry(&entries[i], flags);

        if (entries[i].have_stat && S_ISDIR(entries[i].st.st_mode)) {
            ndirs++;
        }
        else {
            nfiles++;
            if (entries[i].have_stat && S_ISREG(entries[i].st.st_mode)) {
                total += entries[i].st.st_size;
            }
        }

        free(entries[i].name);
    }

    if (!(flags & DIR_BRIEF)) {
        printk("%d director%s, %d file%s, %ld bytes\n",
               ndirs, (ndirs == 1)?"y":"ies", nfiles, (nfiles == 1)?"":"s", total);
    }

    free(entries);
}

void handle_dir_command(int argc, char *argv[]) {
    int flags = 0;
    int npaths = 0;
    int shown = 0;

    // Options first, anywhere on the line
    for (int i=1; i<argc; i++) {
        if (argv[i][0] != '-') {
            npaths++;
            continue;
        }

        for (const char *p = &argv[i][1]; *p != EOS; p++) {
            switch (*p) {
                case 'a':   flags |= DIR_ALL;   break;
                case 'b':   flags |= DIR_BRIEF; break;
                case 'l':   flags &= ~DIR_BRIEF; break;
                default:
                    printk("usage: dir [-a] [-b] [<path> ...]\n");
                    return;
            }
        }
    }

    if (npaths == 0) {
        list_directory(".", flags);
        return;
    }

    for (int i=1; i<argc; i++) {
        struct stat st;
        const char *path = argv[i];

        if (path[0] == '-') {
            continue;
        }

        if (stat(path, &st) != 0) {
            printk("%s: %s\n", path, strerror(errno));
            continue;
        }

        if (npaths > 1) {
            printk("%s%s:\n", (shown > 0)?"\n":"", path);
        }
        shown++;

        if (S_ISDIR(st.st_mode)) {
            list_directory(path, flags);
        }
        else {
            // A file: list just it, by the name it was given
            dir_entry_t e;

            e.name = (char *)path;
            e.st = st;
            e.have_stat = YES;
            print_entry(&e, flags);
        }
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