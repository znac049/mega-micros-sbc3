#pragma once

#define MAXLINE 512
#define MAXARGS 30

#define IS_WHITESPACE(ch) ((ch == ' ') || (ch == '\t'))

#define CURSOR_UP       256
#define CURSOR_DOWN     257
#define CURSOR_LEFT     258
#define CURSOR_RIGHT    259

#define EOS '\0'
#define BS 8
#define LF 10
#define CR 13
#define ESCAPE 27

#define PRINTABLE(ch) (((ch >= ' ') && (ch <= '~'))?ch:'.')

typedef struct command_t {
    char *command;
    int (*fn)(int, char**);
    char *short_help;
} command_t;

extern int g_argc;
extern char *g_argv[MAXARGS];

/* args.c */
int make_args(char *command_line);

/* boot.c */
int do_boot(int argc, char **argv);

/* command.c */
int match_command(int argc, char **argv, command_t cmds[]);
int run_command(int argc, char **argv, command_t cmds[]);