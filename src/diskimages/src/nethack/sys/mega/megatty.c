/* NetHack 3.6	megatty.c	*/
/* NetHack may be freely redistributed.  See license for details. */

/*
 * The terminal, for the Mega-Micros SBC-3: replaces unixtty.c and
 * ioctl.c, and the termcap library.
 *
 * MicroMon's console already gives a raw, unechoed character at a time,
 * so there's no terminal mode to change. There's no termcap database
 * either: tgetent() and friends here answer from one built-in VT100/ANSI
 * entry of 80x24, which the tty window port uses as it would a real one.
 */

#define NEED_VARARGS /* for error() */
#include "hack.h"
#include "tcap.h"

/* Used by win/tty/getline.c: DEL and backspace both erase, ^U kills */
char erase_char = '\177';
char kill_char = '\025';

short ospeed = 0;           /* only used for padding, which there's none of */

static boolean settty_needed = FALSE;

/* -------------------------------------------------------------------- */
/* termcap */

static const struct {
    const char *id;
    const char *value;
} vt100_strings[] = {
    { "cl", "\033[H\033[2J" },  /* clear screen */
    { "ce", "\033[K" },         /* clear to end of line */
    { "cd", "\033[J" },         /* clear to end of screen */
    { "cm", "\033[%i%d;%dH" },  /* cursor motion */
    { "ho", "\033[H" },         /* home */
    { "up", "\033[A" },         /* up a line */
    { "nd", "\033[C" },         /* right one */
    { "xd", "\033[B" },         /* down a line */
    { "le", "\b" },             /* left one */
    { "so", "\033[7m" },        /* standout (reverse) */
    { "se", "\033[m" },
    { "us", "\033[4m" },        /* underline */
    { "ue", "\033[m" },
    { "md", "\033[1m" },        /* bold */
    { "mr", "\033[7m" },        /* reverse */
    { "mb", "\033[5m" },        /* blink */
    { "me", "\033[m" },         /* all attributes off */
    { "as", "\033(0" },         /* DEC line drawing on (DECgraphics) */
    { "ae", "\033(B" },         /* and off */
};

int
tgetent(bp, name)
char *bp;
const char *name;
{
    nhUse(bp);
    nhUse(name);
    return 1; /* found */
}

int
tgetnum(id)
const char *id;
{
    if (!strcmp(id, "co"))
        return 80;
    if (!strcmp(id, "li"))
        return 24;
    return -1;
}

int
tgetflag(id)
const char *id;
{
    nhUse(id);
    return 0;
}

char *
tgetstr(id, area)
const char *id;
char **area;
{
    int i;

    nhUse(area);
    for (i = 0; i < SIZE(vt100_strings); i++)
        if (!strcmp(id, vt100_strings[i].id))
            return (char *) vt100_strings[i].value;

    return (char *) 0;
}

/*
 * Expand a cursor motion string: %d prints a number, %i adds one to both,
 * %r swaps them, %% is a %. As in termcap, the row comes first.
 */
char *
tgoto(cm, col, row)
const char *cm;
int col, row;
{
    static char buf[40];
    char *out = buf;
    int args[2];
    int which = 0;

    args[0] = row;
    args[1] = col;

    while (*cm && out < buf + sizeof buf - 12) {
        if (*cm != '%') {
            *out++ = *cm++;
            continue;
        }

        cm++;
        switch (*cm++) {
        case 'd':
            Sprintf(out, "%d", args[which & 1]);
            out = eos(out);
            which++;
            break;
        case 'i':
            args[0]++;
            args[1]++;
            break;
        case 'r': {
            int t = args[0];

            args[0] = args[1];
            args[1] = t;
            break;
        }
        case '%':
            *out++ = '%';
            break;
        default:
            break;
        }
    }

    *out = '\0';
    return buf;
}

/* Send a string, without the padding at its start (there's no need) */
void
tputs(str, affcnt, outc)
const char *str;
int affcnt;
int FDECL((*outc), (int));
{
    nhUse(affcnt);
    if (!str)
        return;

    while (digit(*str) || *str == '.' || *str == '*')
        str++;

    while (*str)
        (*outc)((unsigned char) *str++);
}

/* -------------------------------------------------------------------- */
/* the terminal's modes */

/* Note the terminal's state, before the game starts using it */
void
gettty()
{
    if (!CO)
        CO = 80;
    if (!LI)
        LI = 24;
    settty_needed = TRUE;
}

/* Put the terminal back as it was */
void
settty(s)
const char *s;
{
    end_screen();
    if (s)
        raw_print(s);
    iflags.echo = OFF;
    iflags.cbreak = ON;
    settty_needed = FALSE;
}

/* Set the terminal up for the game: it's already raw and unechoed */
void
setftty()
{
    iflags.cbreak = ON;
    iflags.echo = OFF;
    start_screen();
}

/* There's no interrupt character to turn on or off */
void
intron()
{
}

void
introff()
{
}

/* unixmain.c asks about a Linux console */
void
check_linux_console()
{
}

void
init_linux_cons()
{
}

/* fatal error */
/*VARARGS1*/
void error
VA_DECL(const char *, s)
{
    VA_START(s);
    VA_INIT(s, const char *);

    if (iflags.window_inited)
        exit_nhwindows((char *) 0); /* for tty, will call settty() */
    if (settty_needed)
        settty((char *) 0);
    Vprintf(s, VA_ARGS);
    (void) putchar('\n');
    VA_END();
    exit(EXIT_FAILURE);
}
