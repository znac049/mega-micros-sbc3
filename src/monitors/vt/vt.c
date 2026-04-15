#include <string.h>
#include <stdio.h>
#include <machine.h>

#include "cli.h"

uint8_t sector_buffer[SECTOR_SIZE];

int do_help(int, char **);

static command_t commands[] = {
    { "ata",        &do_ata, "\tDo stuff with the CompactFlash interface"},
    { "boot",       &do_boot, "[<name>]\tBoot"},
    { "dump",       &do_dump, "[<address> [<count>]]\tDump contents of memory"},
    { "go",         &do_go,   "<address>\tTransfer control to program at <address>"},
    { "load",       &do_load, "[<filename>]\t load filename into memory (default name is 'a.srec')"},
    { "partitions", &do_partitions, "\tSee if there is a partition table on the CF disk"},
    { "srecord",    &do_load_srec, "\tLoad a program in S-Record format"},
    { "help",       &do_help, "[<command>]\tShow help"},
    { NULL, NULL, NULL}
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
    printf("The following commands are available:\r\n");

    for (int i=0; commands[i].command != NULL; i++) {
        printf("  %s %s\r\n", commands[i].command, commands[i].short_help);
    }

    printf("\r\n");

    return 0;
}

void main(void) {
    char cmd[MAXLINE];
    short running = 1;
    int res;

    while (running) {
        int argc;
        int num_matches;

        printf("-> ");
        res = readline(cmd, MAXLINE);
        if (res == 0) {
            running = 0;
        }
        else {
            argc = make_args(cmd);

            num_matches = match_command(argc, g_argv, commands);
            printf("\r\nnum matches = %d\r\n", num_matches);

            if (num_matches > 1) {
                printf("Command '%s' is ambiguous.\r\n", g_argv[0]);
            }
            else if (num_matches == 1) {
                res = run_command(argc, g_argv, commands);
                if (res) {
                    printf("Command exited with error code %d\r\n", res);
                }
            }
            else {
                printf("'%s' is not a command.\r\n", g_argv[0]);
            }
        }
    }
}