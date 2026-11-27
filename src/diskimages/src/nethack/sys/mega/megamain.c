/* NetHack 3.6	megamain.c	*/
/* NetHack may be freely redistributed.  See license for details. */

/*
 * The start of NetHack on the Mega-Micros SBC-3. MicroMon passes no
 * environment, so this sets one up for unixmain.c's main(), which is
 * compiled as unix_main() (see the Makefile):
 *
 *   NETHACKDIR   where the game's files are: <disk>/games/nethack when
 *                the game is <disk>/bin/nethack, wherever <disk> is
 *                mounted (-d <dir> on the command line overrides it)
 *   HOME         the same, for the options file (.nethackrc)
 *   USER         the one player there is
 *   TERM         vt100 (see megatty.c)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "megaconf.h"

int unix_main(int argc, char **argv);

/* Strip the last part of a path ("/a/b/c" -> "/a/b"); "/a" -> "/" */
static void
strip_last(path)
char *path;
{
    char *slash = strrchr(path, '/');

    if (slash == path)
        slash[1] = '\0';
    else if (slash)
        *slash = '\0';
}

int
main(argc, argv)
int argc;
char **argv;
{
    char dir[PATH_MAX];

    /* <disk>/bin/nethack -> <disk>/games/nethack */
    if (argc > 0 && argv[0] && realpath(argv[0], dir) != NULL) {
        strip_last(dir); /* <disk>/bin */
        strip_last(dir); /* <disk> */
        if (strlen(dir) + sizeof "/games/nethack" < sizeof dir) {
            if (strcmp(dir, "/") != 0)
                strcat(dir, "/games/nethack");
            else
                strcpy(dir, "/games/nethack");
            setenv("NETHACKDIR", dir, 0);
            setenv("HOME", dir, 0);
        }
    }

    setenv("USER", MEGA_USER, 0);
    setenv("TERM", "vt100", 0);

    return unix_main(argc, argv);
}
