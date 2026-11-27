/* NetHack 3.6	megaconf.h	*/
/* NetHack may be freely redistributed.  See license for details. */

/*
 * Configuration for the Mega-Micros SBC-3 (68030) running MicroMon.
 *
 * Included by unixconf.h in place of its choice of Unix flavour. The
 * board is "UNIX" as far as NetHack is concerned - files, directories,
 * a raw serial console - and its C library follows POSIX closely enough
 * to take the System V paths. But there's no termcap or terminfo, job
 * control, mail, shell, compress or hard links.
 *
 * The terminal is taken to be a VT100/ANSI one of 80x24 (sys/mega/megatty.c
 * supplies a built-in termcap entry for it).
 */

#ifndef MEGACONF_H
#define MEGACONF_H

#define SYSV            /* closest: the System V/POSIX code paths and types */
#define POSIX_TYPES     /* use POSIX types for system calls */
#define TEXTCOLOR       /* ANSI colours */
#define NO_FILE_LINKS   /* no link(): lock with exclusive creates instead */
#define NOMAIL          /* no mail daemon */
#define NOSHELL         /* no '!' command */
#define DLB             /* data files packed into one, nhdat */

/* compress and gzip don't exist here: save files are written as they are */
#undef COMPRESS
#undef COMPRESS_EXTENSION

/* The board has no users: everyone plays as one */
#define MEGA_USER   "player"

#endif /* MEGACONF_H */
