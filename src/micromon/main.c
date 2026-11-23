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
#include <sys/stat.h>

#include "micromon.h"
#include "expr.h"


static void print_help(void);

static command_t commands[] = {
    {"help",        2, COMMAND, {print_help},               NONE},
    {"cat",         0, COMMAND, {handle_cat_command},       VEC},
    {"cd",          0, COMMAND, {handle_cd_command},        VEC},
    {"cftest",      0, COMMAND, {handle_cftest_command},    VEC},
    {"dir",         0, COMMAND, {handle_dir_command},       VEC},
    {"disassemble", 3, COMMAND, {handle_disasm_command},    VEC},
    {"e2test",      0, COMMAND, {handle_e2test_command},    VEC},
    {"eval",        2, COMMAND, {handle_eval_command},      STR},
    {"go",          0, COMMAND, {handle_go_command},        VEC},
    {"load",        2, COMMAND, {handle_load_command},      VEC},
    {"memtest",     4, COMMAND, {handle_memtest_command},   VEC},
    {"probe",       3, COMMAND, {handle_probe_command},     VEC},
    {"pwd",         0, COMMAND, {handle_pwd_command},       VEC},
    {"rtc",         3, COMMAND, {handle_rtc_command},       VEC},
    {"run",         2, COMMAND, {handle_run_command},       VEC},
    {"ser1",        0, COMMAND, {handle_ser_command},       VEC},
    {"ser2",        0, COMMAND, {handle_ser_command},       VEC},
    {"byteset",     6, COMMAND, {handle_setbyte_cmd},   STR},
    {"wordset",     6, COMMAND, {handle_setword_cmd},   STR},
    {"longset",     6, COMMAND, {handle_setlong_cmd},   STR},
    {"bytedump",    6, COMMAND, {handle_bytedump_command},  VEC},
    {"worddump",    6, COMMAND, {handle_worddump_command},  VEC},
    {"longdump",    6, COMMAND, {handle_longdump_command},  VEC},
    {"showmount",   5, COMMAND, {handle_showmount_command}, VEC},
    {"icache",      3, COMMAND, {handle_icache_cmd},    VEC},
    {"dcache",      3, COMMAND, {handle_dcache_cmd},    VEC},

    {"sb",          2, ALIAS,   {.cmd = "byteset"},         STR},
    {"sw",          2, ALIAS,   {.cmd = "wordset"},         STR},
    {"sl",          2, ALIAS,   {.cmd = "longset"},         STR},

    {"dump",        2, ALIAS,   {.cmd = "bytedump"},        VEC},
    {"db",          2, ALIAS,   {.cmd = "bytedump"},        VEC},
    {"dw",          2, ALIAS,   {.cmd = "worddump"},        VEC},
    {"dl",          2, ALIAS,   {.cmd = "longdump"},        VEC},

    {"ls",          2, ALIAS,   {.cmd = "dir"},             VEC},

    {"=",           1, PREFIX,  {.cmd = "eval"},            STR},
    {"!",           1, PREFIX,  {.cmd = "byteset"},         STR},
    {"?",           1, PREFIX,  {.cmd = "bytedump"},        VEC},
};

#define NUM_COMMANDS (sizeof(commands)/sizeof(command_t))


static char *aye[] = {"yes", "true", "on", "enable"};
static char *nay[] = {"no", "false", "off", "disable"};

int parse_truth(char *val) {
    for (uint32_t i=0; i<(sizeof(aye)/sizeof(char *)); i++) {
        if (strcasecmp(val, aye[i]) == 0) {
            return YES;
        }
    }

    for (uint32_t i=0; i<(sizeof(nay)/sizeof(char *)); i++) {
        if (strcasecmp(val, nay[i]) == 0) {
            return NO;
        }
    }

    return NOT_OK;
}

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

static void print_help(void) {
    printk("Commands are:\n");
    printk("  byteedit [<address>] [[=] <expr> [, <expr> ...]]\n");
    printk("  cat <filename>\n");
    printk("  cd <path>\n");
    printk("  cftest <drive> <sector>\n");
    printk("  e2test <mount>\n");
    printk("  dir [ <path> ]\n");
    printk("  disassemble <address>\n");
    printk("  dump [<start_address> [<count>]]\n");
    printk("  eval <expression>\n");
    printk("  go <address>]\n");
    printk("  load\n");
    printk("  longedit [<address>] [[=] <expr> [, <expr> ...]]\n");
    printk("  memtest [<start> <end> [<passes>]]\n");
    printk("  probe\n");
    printk("  rtc (erase) | (time [hh:mm[:ss]]) | (date [yyyy:mm:dd])\n");
    printk("  run <file> [<args> ...]\n");
    printk("  ser1|2 baud <baudrate>\n");
    printk("  wordedit [<address>] [[=] <expr> [, <expr> ...]]\n");
    printk("  quit\n\n");
}

static command_t *find_command(char *cmd) {
    // printk("find_command: looking up '%s'\n", cmd);

    for (unsigned int i=0; i<NUM_COMMANDS; i++) {
        if (is_command(cmd, commands[i].command, commands[i].min_required, commands[i].type) == YES) {
            return &commands[i];
        }
    }

    return NULL;
}

static int find_command_on_disk(char *cmdline) {
    char *argv[MAX_ARGS];
    int argc = split_str(cmdline, ' ', argv, MAX_ARGS);
    char name[PATH_MAX];
    char pwd[PATH_MAX];
    struct stat sb;

    if (argc == 0) {
        return NOT_OK;
    }

    /*
     * Keep is simple, so look 
     * - in the current directory
     * - in /bin on every mountpoint, in mount
     *   table order
     */

    if (getcwd(pwd, sizeof(name)) != NULL) {
        snprintf(name, sizeof(name), "/%s/%s", pwd, argv[0]);

        if (stat(name, &sb) == OK) {
            // Something found. Check it's a file
            if (S_ISREG(sb.st_mode)) {
                int fd = open(name, O_RDONLY);

                return fd;
            }
        }
    }

    for (int i=0; i<MAX_MOUNTS; i++) {
        vmp_t *mp = vfs_get_mount(i);

        if ((mp != NULL) && (mp->mounted == YES)) {
            snprintf(name, sizeof(name), "/%s/bin/%s", mp->name, argv[0]);

            if (stat(name, &sb) == OK) {
                // Something found. Check it's a file
                if (S_ISREG(sb.st_mode)) {
                    int fd = open(name, O_RDONLY);

                    return fd;
                }
            }
        }
    }

    return NOT_OK;
}

static void handle_command(char *cmdline) {
    command_t *cmd;
    char cmdstr[MAX_LINE];
    char *cp;

    strcpy(cmdstr, cmdline);
    cp = strchr(cmdstr, ' ');
    if (cp != NULL) {
        *cp = EOS;
    }

    if (strblank(cmdstr) == YES) {
        return;
    }

    cmd = find_command(cmdstr);

    if (cmd == NULL) {
        int res;
        uint32_t entry;
        char *argv[MAX_ARGS];
        int argc;

        // It's not a builtin command. Search the disk
        int fd = find_command_on_disk(cmdstr);
        if (fd == NOT_OK) {
            printk("'%s': unrecognised command\n", cmdstr);
            return;
        }

        res = load_elf(fd, &entry);
        close(fd);

        if (res == NOT_OK) {
            printk("%s: not loaded\n", argv[1]);
            return;
        }
    
        go_address = entry;
        argc = split_str(cmdline, ' ', argv, MAX_ARGS);
        go_run(argc, argv);
    }
    else {
        /* 
        * We've found a builtin command, but it might be an alias
        * or a prefix (or NULL)
        */
        while ((cmd->type != COMMAND)) {
            // printk("handle_command: found %s '%s'\n",
            //     (cmd->type == ALIAS)?"alias":"prefix", cmd->command);
            cmd = find_command(cmd->cmd);
        }

        if (cmd == NULL) {
            printk("'%s': Unrecognised command.\n", cmdstr);
            return;
        }

        // printk("handle_command: calling handler for'%s'\n", cmd->command);

        switch (cmd->args) {
            case NONE:
                cmd->handler();
                break;

            case STR:
                printk("Passing command line: '%s'\n", cmdline);
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
}

void main(void) {
    char cmdline[MAX_LINE];
    char cwd[PATH_MAX];

    setup();

    printk("Ready.\n");

    while (1) {
        if (getcwd(cwd, PATH_MAX) == NULL) {
            strcpy(cwd, "");
        }

        printk("\r%s# ", cwd);

        if (kgets(cmdline) != NULL) {
            printk("\r");
            handle_command(cmdline);
        }
    }
}