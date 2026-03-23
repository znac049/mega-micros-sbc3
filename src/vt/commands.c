#include <stddef.h>
#include "string.h"
#include "cli.h"
#include "printf.h"

int could_it_be(char *partial, char *command) {
    char ch;

    //printf("\r\ncould '%s' be '%s'???\r\n", partial, command);
    if (strlen(partial) > strlen(command)) {
        return 0;
    }

    while ((ch = *partial++) != EOS) {
        if (ch != *command++) {
            return 0;
        }
    }

    return 1;
}

int match_command(int argc, char **argv, command_t cmds[]) {
    int num_matches = 0;

    for (int i=0; cmds[i].command != NULL; i++) {
        if (could_it_be(argv[0], cmds[i].command)) {
            num_matches++;
        }
    }

    return num_matches;
}

int run_command(int argc, char **argv, command_t cmds[]) {
    int num_matches = 0;
    command_t *match=NULL;

    for (int i=0; cmds[i].command != NULL; i++) {
        if (could_it_be(argv[0], cmds[i].command)) {
            num_matches++;
            match = &cmds[i];
        }
    }

    if (num_matches == 1) {
        printf("Running the '%s' command\r\n", match->command);
        return match->fn(argc, argv);
    }

    return -1;
}