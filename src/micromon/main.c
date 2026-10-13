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
    {"help",        2, COMMAND, {print_help}},
    {"cat",         0, COMMAND, {handle_cat_command}},
    {"cd",          0, COMMAND, {handle_cd_command}},
    {"dir",         0, COMMAND, {handle_dir_command}},
    {"disassemble", 3, COMMAND, {handle_disasm_command}},
    {"dump",        2, COMMAND, {handle_dump_command}},
    {"eval",        2, COMMAND, {handle_eval_command}},
    {"go",          0, COMMAND, {handle_go_command}},
    {"load",        2, COMMAND, {handle_load_command}},
    {"probe",       3, COMMAND, {handle_probe_command}},
    {"pwd",         0, COMMAND, {handle_pwd_command}},
    {"rtc",         3, COMMAND, {handle_rtc_command}},
    {"ser1",        0, COMMAND, {handle_ser_command}},
    {"ser2",        0, COMMAND, {handle_ser_command}},
    {"byteedit",    3, COMMAND, {handle_editb_command}},
    {"wordedit",    3, COMMAND, {handle_editw_command}},
    {"longedit",    3, COMMAND, {handle_editl_command}},

    {"eb"           2, ALIAS,   {.cmd = "byteedit"}},
    {"ew"           2, ALIAS,   {.cmd = "wordedit"}},
    {"el"           2, ALIAS,   {.cmd = "longedit"}},

    {"=",           1, PREFIX,  {.cmd = "eval"}},
    {"@",           1, PREFIX,  {.cmd = "byteedit"}},
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

bool_t is_blank(const char *str) {
    while (*str) {
        if (!isblank(*str++)) {
            return NO;
        }
    }

    return YES;
}

void print_help(void) {
    kprintf("Commands are:\n");
    kprintf("  cat <filename>\n");
    kprintf("  cd <path>\n");
    kprintf("  dir [ <path> ]\n");
    kprintf("  disassemble <address>\n");
    kprintf("  dump [<start_address> [<count>]]\n");
    kprintf("  eval <expression>\n");
    kprintf("  go <address>]\n");
    kprintf("  load\n");
    kprintf("  probe\n");
    kprintf("  rtc (erase) | (time [hh:mm[:ss]]) | (date [yyyy:mm:dd])\n");
    kprintf("  ser1|2 baud <baudrate>\n");
    kprintf("  quit\n\n");
}

command_t *find_command(char *cmdline) {
    for (unsigned int i=0; i<NUM_COMMANDS; i++) {
        if (is_command(cmdline, commands[i].command, commands[i].min_required, commands[i].type) == YES) {
            return &commands[i];
        }
    }

    return NULL;
}

void handle_command(int argc, char *argv[], char *cmdline) {
    register const char *cmdname = argv[0];
    command_t *cmd;

    if (is_blank(cmdname) == YES) {
        return;
    }

    cmd = find_command(cmdline);
    while ((cmd != NULL) && (cmd->type != COMMAND)) {
        cmd = find_command(cmd->cmd);
    }

    /* We've found the command */
    if (cmd == NULL) {
        kprintf("'%s': Unrecognised command.\n", cmdname);
    }
    else {
        cmd->handler(cmdline, argc, argv);
    }
}

void main(void) {
    char cmdline[MAX_LINE];
    char dup_cmd[MAX_LINE];
    char cwd[PATH_MAX];

    setup();

    while (1) {
        if (getcwd(cwd, PATH_MAX) == NULL) {
            strcpy(cwd, "");
        }

        kprintf("%s# ", cwd);

        if (kgets(cmdline) != NULL) {
            char *argv[MAX_ARGS];
            int argc;
            
            strncpy(dup_cmd, cmdline, sizeof(dup_cmd));
            argc = split_str(dup_cmd, ' ', argv, MAX_ARGS);

            if (argc) {
                handle_command(argc, argv, cmdline);
            }
        }
    }
}