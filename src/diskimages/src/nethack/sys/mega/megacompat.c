/* NetHack 3.6	megacompat.c	*/
/* NetHack may be freely redistributed.  See license for details. */

/*
 * Unix calls NetHack's Unix code makes that MicroMon's C library doesn't
 * have, because the board has no users, hard links or child processes.
 */

#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <pwd.h>

#include "megaconf.h"

static struct passwd the_user = {
    MEGA_USER, "", 0, 0, MEGA_USER, "/", ""
};

struct passwd *
getpwuid(uid)
uid_t uid;
{
    return (uid == 0) ? &the_user : (struct passwd *) 0;
}

struct passwd *
getpwnam(name)
const char *name;
{
    return (name && !strcmp(name, MEGA_USER)) ? &the_user : (struct passwd *) 0;
}

char *
getlogin()
{
    return MEGA_USER;
}

/* No hard links (NetHack locks with fcntl() instead) */
int
link(oldpath, newpath)
const char *oldpath, *newpath;
{
    (void) oldpath;
    (void) newpath;
    errno = EPERM;
    return -1;
}

/* No child processes to wait for */
int
wait(status)
int *status;
{
    (void) status;
    errno = ECHILD;
    return -1;
}

pid_t
waitpid(pid, status, options)
pid_t pid;
int *status, options;
{
    (void) pid;
    (void) status;
    (void) options;
    errno = ECHILD;
    return -1;
}
