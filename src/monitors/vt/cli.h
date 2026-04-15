#pragma once

#include <ctype.h>

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

#define SECTOR_SIZE 512

#define PART_NONE       0
#define PART_FAT16      1
#define PART_MINIXV1    2

typedef struct command {
    char *command;
    int (*fn)(int, char**);
    char *short_help;
} command_t;

typedef struct partition {
    uint8_t status;
    uint8_t chs_start[3];
    uint8_t type;
    uint8_t chs_end[3];
    uint32_t lba_start;
    uint32_t num_sectors;
} partition_t;

typedef struct MBR {
    uint8_t boot_code[446];
    partition_t partition_table[4];
    uint16_t signature;
} MBR_t;

/* args.c */
int make_args(char *command_line);

/* boot.c */
int do_boot(int argc, char **argv);

/* cf.c */
int do_ata(int argc, char **argv);
int do_partitions(int argc, char **argv);
void dump_sector(uint32_t sector_num);

/* command.c */
int match_command(int argc, char **argv, command_t cmds[]);
int run_command(int argc, char **argv, command_t cmds[]);

/* dump.c */
int do_dump(int argc, char **argv);
void dump_buffer(uint8_t *buffer, int buffer_len);
int is_printable(char ch);

/* fat16.c */
int open_fat16_partition(int disk_id, int partition_number);

/* go.c */
int do_go(int argc, char **argv);

/* load.c */
int do_load(int argc, char **argv);

/* partitions.c */
int get_partition_type(int disk_id, int partition_num);
int has_mbr(int disk_id);

/* srec */
int do_load_srec(int argc, char **argv);

/* utils.c */
int hexval(char c);
int is_hex_char(char c);
void ltrim(char *str);
void rtrim(char *str);
void trim(char *str);

/* vt.c */
extern int g_argc;
extern char *g_argv[MAXARGS];
extern uint8_t sector_buffer[SECTOR_SIZE];