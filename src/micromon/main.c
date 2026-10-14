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

#include <stdio.h>
#include <ctype.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <extras.h>
#include <unistd.h>
#include <limits.h>
#include <machine.h>

#include "micromon.h"
#include "expr.h"


void print_help(void);

static command_t commands[] = {
    {"help",        2, COMMAND, {print_help},               NONE},
    {"cat",         0, COMMAND, {handle_cat_command},       VEC},
    {"cd",          0, COMMAND, {handle_cd_command},        VEC},
    {"dir",         0, COMMAND, {handle_dir_command},       VEC},
    {"disassemble", 3, COMMAND, {handle_disasm_command},    VEC},
    {"dump",        2, COMMAND, {handle_dump_command},      VEC},
    {"eval",        2, COMMAND, {handle_eval_command},      STR},
    {"go",          0, COMMAND, {handle_go_command},        VEC},
    {"load",        2, COMMAND, {handle_load_command},      VEC},
    {"probe",       3, COMMAND, {handle_probe_command},     VEC},
    {"pwd",         0, COMMAND, {handle_pwd_command},       VEC},
    {"rtc",         3, COMMAND, {handle_rtc_command},       VEC},
    {"ser1",        0, COMMAND, {handle_ser_command},       VEC},
    {"ser2",        0, COMMAND, {handle_ser_command},       VEC},
    {"byteset",     3, COMMAND, {handle_setbyte_command},   STR},
    {"wordset",     3, COMMAND, {handle_setword_command},   STR},
    {"longset",     3, COMMAND, {handle_setlong_command},   STR},

    {"bs",          2, ALIAS,   {.cmd = "byteset"},         STR},
    {"ws",          2, ALIAS,   {.cmd = "wordset"},         STR},
    {"ls",          2, ALIAS,   {.cmd = "longset"},         STR},

    {"=",           1, PREFIX,  {.cmd = "eval"},            STR},
    {"@",           1, PREFIX,  {.cmd = "byteedit"},        STR},
};

#define NUM_COMMANDS (sizeof(commands)/sizeof(command_t))

bool_t is_command(const char *cmd, const char *target, int min_target_len, cmd_type_t type) {
    int cmd_len = strlen(cmd);
    int target_len = strlen(target);

    if (type == PREFIX) {
        cmd_len = min_target_len;
    }

    if (min_target_len == 0) {
        min_target_len = target_len;
    }

    if ((cmd_len > target_len) || (cmd_len < min_target_len)) {
        return NO;
    }

    for (int i=0; i<cmd_len; i++) {
        if (tolower(cmd[i]) != tolower(target[i])) {
            return NO;
        }
    }

    return YES;
}

void print_help(void) {
    kprintf("Commands are:\n");
    kprintf("  byteedit [<address>] [[=] <expr> [, <expr> ...]]\n");
    kprintf("  cat <filename>\n");
    kprintf("  cd <path>\n");
    kprintf("  dir [ <path> ]\n");
    kprintf("  disassemble <address>\n");
    kprintf("  dump [<start_address> [<count>]]\n");
    kprintf("  eval <expression>\n");
    kprintf("  go <address>]\n");
    kprintf("  load\n");
    kprintf("  longedit [<address>] [[=] <expr> [, <expr> ...]]\n");
    kprintf("  probe\n");
    kprintf("  rtc (erase) | (time [hh:mm[:ss]]) | (date [yyyy:mm:dd])\n");
    kprintf("  ser1|2 baud <baudrate>\n");
    kprintf("  wordedit [<address>] [[=] <expr> [, <expr> ...]]\n");
    kprintf("  quit\n\n");
}

command_t *find_command(char *cmdline) {
    kprintf("find_command: looking up '%s'\n", cmdline);

    for (unsigned int i=0; i<NUM_COMMANDS; i++) {
        if (is_command(cmdline, commands[i].command, commands[i].min_required, commands[i].type) == YES) {
            return &commands[i];
        }
    }

    return NULL;
}

void handle_command(char *cmdline) {
    command_t *cmd;
    char cmdstr[MAX_LINE];
    char *cp;

    strcpy(cmdstr, cmdline);
    cp =strchr(cmdstr, ' ');
    if (cp != NULL) {
        *cp = EOS;
    }

    if (strblank(cmdstr) == YES) {
        return;
    }

    cmd = find_command(cmdstr);
    /* 
     * We've found the command, but it might be an alias
     * or a prefix (or NULL)
     */
    while ((cmd != NULL) && (cmd->type != COMMAND)) {
        kprintf("handle_command: found %s '%s'\n",
            (cmd->type == ALIAS)?"alias":"prefix", cmd->command);
        cmd = find_command(cmd->cmd);
    }

    if (cmd == NULL) {
        kprintf("'%s': Unrecognised command.\n", cmdstr);
        return;
    }

    kprintf("handle_command: calling handler for'%s'\n", cmd->command);

    switch (cmd->args) {
        case NONE:
            cmd->handler();
            break;

        case STR:
            cmd->handler(cmdline);
            break;
        
        case VEC:
            {
                char *argv[MAX_ARGS];
                int argc = split_str(cmdline, ' ', argv, MAX_ARGS);

                cmd->handler(argc, argv);
            }
            break;

        case BOTH:
            {
                char *argv[MAX_ARGS];
                int argc = split_str(cmdline, ' ', argv, MAX_ARGS);

                cmd->handler(argc, argv, cmdline);
            }
            break;
    }
}

void main(void) {
    char cmdline[MAX_LINE];
    char cwd[PATH_MAX];

    setup();

    while (1) {
        if (getcwd(cwd, PATH_MAX) == NULL) {
            strcpy(cwd, "");
        }

        kprintf("%s# ", cwd);

        if (kgets(cmdline) != NULL) {
            handle_command(cmdline);
        }
    }
}