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

#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

// The handler for each signal (SIG_DFL to begin with)
static sighandler_t handlers[_NSIG + 1];

sighandler_t signal(int signum, sighandler_t handler) {
    sighandler_t old;

    // SIGKILL can't be caught or ignored
    if ((signum <= 0) || (signum > _NSIG) || (signum == SIGKILL)) {
        errno = EINVAL;
        return SIG_ERR;
    }

    old = handlers[signum];
    handlers[signum] = handler;

    return old;
}

/*
 * Deliver a signal to this program: run its handler, do nothing if it's
 * ignored, or by default end the program (except for SIGCHLD, whose
 * default is to be ignored).
 */
int raise(int sig) {
    sighandler_t handler;

    if ((sig <= 0) || (sig > _NSIG)) {
        errno = EINVAL;
        return -1;
    }

    handler = handlers[sig];

    if (handler == SIG_IGN) {
        return 0;
    }

    if (handler == SIG_DFL) {
        if (sig == SIGCHLD) {
            return 0;
        }

#if !defined(BAREMETAL)
        exit(128 + sig);
#else
        // The monitor itself can't exit: stop here
        for (;;) {
        }
#endif
    }

    // As on Unix, the handler goes back to the default before it runs
    handlers[sig] = SIG_DFL;
    handler(sig);

    return 0;
}

// The only process there is to signal is this one
int kill(pid_t pid, int sig) {
    if ((pid != 0) && (pid != getpid())) {
        errno = ESRCH;
        return -1;
    }

    if (sig == 0) {
        return 0;               // just checking it exists
    }

    return raise(sig);
}
