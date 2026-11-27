/* NetHack 3.6	sys/wait.h for the Mega-Micros SBC-3 */
/* NetHack may be freely redistributed.  See license for details. */

/* There are no child processes on the board: just enough for NetHack */

#ifndef MEGA_SYS_WAIT_H
#define MEGA_SYS_WAIT_H

#include <sys/types.h>

#define WNOHANG     1

int wait(int *status);
pid_t waitpid(pid_t pid, int *status, int options);

#endif
