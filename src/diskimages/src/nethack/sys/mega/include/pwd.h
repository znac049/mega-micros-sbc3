/* NetHack 3.6	pwd.h for the Mega-Micros SBC-3 */
/* NetHack may be freely redistributed.  See license for details. */

/* The board has one user (see megacompat.c) */

#ifndef MEGA_PWD_H
#define MEGA_PWD_H

#include <sys/types.h>

struct passwd {
    char *pw_name;
    char *pw_passwd;
    uid_t pw_uid;
    gid_t pw_gid;
    char *pw_gecos;
    char *pw_dir;
    char *pw_shell;
};

struct passwd *getpwuid(uid_t uid);
struct passwd *getpwnam(const char *name);

#endif
