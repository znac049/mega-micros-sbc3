#include "string.h"
#include "cli.h"

int g_argc;
char *g_argv[MAXARGS];
static char s_command[MAXLINE];

int make_args(char *command_line) {
    char *start = s_command;
    char *end;
    
    strcpy(s_command, command_line);

    g_argc = 0;
    while (1) {
        char ch = *start;

        // Skip leading whitespace
        while (IS_WHITESPACE(ch)) {
            start++;
            ch = *start;
        }

        g_argv[g_argc++] = start;
        end = start;
        ch = *end;
        while (!IS_WHITESPACE(ch)) {
            end++;
            ch = *end;

            if (ch == EOS) {
                return g_argc;
            }
        }

        *end++ = EOS;
        start = end;
    }

    return g_argc;
}
