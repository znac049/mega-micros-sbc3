#include "printf.h"
#include "machine.h"

#define MAXLINE 512

#define MAXSEQ 16

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

int do_help(int, char **);

static struct command_t {
    char *command;
    int (*fn)(int, char**);
} commands[] = {
    {
        "help", &do_help
    }
};

int get_vt_char(void) {
    int ch;
    ch = _getchar();

    while (1) {
        //printf("%02x - '%c'\r\n", ch, PRINTABLE(ch));

        switch (ch) {
            case ESCAPE:
                // It's an escape sequence
                ch = _getchar();
                printf(" - %02x - '%c'\r\n", ch, PRINTABLE(ch));
                if (ch == '[') {
                    ch = _getchar();
                    printf(" - %02x - '%c'\r\n", ch, PRINTABLE(ch));

                    switch (ch) {
                        case 'A':
                            ch = CURSOR_UP;
                            break;

                        case 'B':
                            ch = CURSOR_DOWN;
                            break;

                        case 'C':
                            ch = CURSOR_RIGHT;
                            break;

                        case 'D':
                            ch = CURSOR_LEFT;
                            break;

                        default:
                            break;
                    }

                    return ch;

                }
                break;

            default:
                return ch;
        }
    }

    return ch;
}

int readline(char *line, int max_len) {
    int ch;
    int i = 0;

    while ((ch = get_vt_char())) {
        switch(ch) {
            case CR: case LF:
                line[i] = EOS;
                return i;

            case BS:
                if (i) {
                    _putchar(BS);
                    _putchar(' ');
                    _putchar(BS);
                    i--;
                }
                break;

            default:
                _putchar((char)(ch & 0xff));
                line[i++] = (char)(ch & 0xff);
                if (i >= max_len) {
                    return -1;
                }
                break;
        }
    }

    return -1;
}

int do_help(int argc, char **argv) {
    return 1;
}

void main(void) {
    char cmd[MAXLINE];

    printf("Command are:\r\n");
    for (unsigned int i=0; i<(sizeof(commands) / sizeof(struct command_t)); i++) {
        printf("  %s\r\n", commands[i].command);
    }

    while (readline(cmd, MAXLINE)) {
        printf("\r\nGot commandline: '%s'\r\n", cmd);
    }
}