# MicroMon User Guide
This manual provides full documentation for the MicroMon monitor for the [Mega-Micros 68030 SBC-3 board](https://mega-micros.co.uk/index_68030.htm).

MicroMon is an alternative monitor to the one supplied with the board, which better suited my needs and also gave me the opportunity to really get to grips with understanding the board and it's hardware.

It is written predominantly in C with a minimal amount of assembly language.

## Features
- Takes care of all low-level initialisation of the board.
  - Detects the amount of memory
  - Detects which duart variant is being used and if it is being overclocked.
- Allows memory to be inspected and edited
- Allows programs to be loaded and then run via serial port, in [Motorola S-Record format](https://en.wikipedia.org/wiki/Motorola_S-record)
- Provides a number of bios/system calls for the user program which provide many low level system access, including:
  - Both serial ports
  - LED control
  - Access to the onboard DS-1307 real time clock.
  - Compact Flash (ext2 filesystem)
  - ROM based disk (ext2 filesystem)
- ROM validity checked on reset.
- Memory test.
- Alongside the monitor a C runtime library is included which provided a comprehensive (and getting more complete every day) set of many of the standard functions you'd expect to find on any linux or unix system.

## Startup
### Startup errors
How startup errors are displayed depends on how early in the initialisation code they occur. If an error occurs before the duart has been initialise, LEDs and the Hex Display card will be used to indicate what has occurred. If the system has initialised the duart, it will display an appropriate error message on the console serial port.


## Monitor Commands

### help - print help message
Simply prints a brief summary of each command the monitor accepts.

### byteset, wordset, longset - edit data in memory
__Aliases__: **sb**, **sw**, **sl**
__Shortform__: **!**

```
sb [ = <expr> [, <expr> ...]]
sb <address> [ [ = ] <expr> [, <expr> ...]]
![b|w|l] [ <address> ] [ [ = ] <expr> [, <expr> ...]]
```

### bytedump, worddump, longdump - dump contents of memory
__Aliases__: **dump**, **bd**, **wd**, **ld**
__Shortform__: **?**
```
wd [ <address> [ <number_of_items> ]]
```

### cat - display the contents of a file
#### Synopsis
```
cat <filename>
``` 
#### Details
The file specified by **_<filename>_** is displayed on the console. If **_<filename>_** does not contain a path part, it will be read from the current directory.

### cd - change directory
#### Synopsis
```
```

### chmod - change a file's permissions
#### Synopsis
```
chmod <octal mode> <file> [<file> ...]
```
#### Details
Sets the permission bits of each **_<file>_** (or directory) to **_<octal mode>_**, given in octal as for the Unix command: `755`, `0644`, `4755`... The file's type is unchanged. The filesystem must be writable, so not the ROM disk.

### dcache - control the data cache
#### Synopsis
```
dcache [ <on> | <off> ]
```
#### Details

### dir - list the contents of a directory
__Alias__: **ls**
#### Synopsis
```
dir [-a] [-b] [<path> ...]
```
#### Details
Lists each directory given, or the current directory, sorted by name. For each entry it shows the type and permissions, the number of links, the size in bytes, the time it was last changed (UTC) and the name, with a `/` after directories:
```
drwxr-xr-x   2      2048 2026-11-26 14:24  bin/
-rwxr-xr-x   1    412345 2026-11-26 14:24  vasm
-rw-r--r--   1        62 -                 motd
```
A time of `-` means it was never set (it was written without the RTC). A summary of the number of directories and files, and their total size, follows. A file given as **_<path>_** is listed on its own line.

- **-b** lists just the names (brief), with `/` after directories
- **-a** also lists the `.` and `..` entries

### disassemble - disassemble machine code
#### Synopsis
```
```

### eval - evaluate expressions
__Shortform__: **=**
#### Synopsis
```
eval <expression> [, <expression> ... ]
```
#### Details
Evaluate one or more integer expressions. Numbers can be entered in hexadecimal (prefix with '$' or '0x') or decimal. Operators accepted are:
```
+
-
*
/
<<
>>
%
```
The result is printed in both decimal and hexadecimal.

The following constants can be used:
```
ACRTC_BASE
DAC_BASE
DUART_BASE
CF_BASE
HEX_DISPLAY_BASE
PIT_BASE
ROM_BASE
RAM_BASE
```

__Example__
```
/# eval ROM_BASE+27
-> 12582939 (0x00c0001b)
/#
```

To minimise typing, an alias exists, ```=``` which can be used instead:
```
/# =7*6
-> 42 (0x0000002a)
/~
```
Note that there is no need for a space between the ```=``` and the expression.

### go - run a user program
#### Synopsis
```
go [ <address> ]
```
#### Details
Transfers control to a user program. If **_<address>_** is omitted, the load address from the last **_load_** command is used.

### icache - control the instruction cache
#### Synopsis
```
icache [ <on> | <off> ]
```
#### Details

### load - load a program from the console port.
#### Synopsis
```
load
```
#### Details
The monitor loads a program, in Motorola S-Record format, from the console port. A countdown is displayed and if no data has been received when it hits zero, the load is aborted. If a start address record is present, it will be remembered and used by the **_go_** command, if no other address is specified.

### mkdir - make directories
#### Synopsis
```
mkdir [-p] <directory> [<directory> ...]
```
#### Details
Makes each **_<directory>_**, with permissions 755. The directory it goes in must already exist, unless **-p** is given: then any missing parents are made too, and a directory that's already there isn't an error. The filesystem must be writable, so not the ROM disk.

### probe
#### Synopsis
```
```

### pwd - print current directory
#### Synopsis
```
pwd
```

### rm - delete files
#### Synopsis
```
rm <file> [<file> ...]
```
#### Details
Deletes each **_<file>_**. A directory needs **rmdir**, and a file that's open (by a program that's still running, for instance) can't be deleted. If the file has other names (hard links), only this name goes. The filesystem must be writable, so not the ROM disk.

### rmdir - delete empty directories
#### Synopsis
```
rmdir <directory> [<directory> ...]
```
#### Details
Deletes each **_<directory>_**, which must be empty. The current directory, and the top directory of a mount, can't be deleted.

### rtc - configure/show clock settings
#### Synopsis
```
```

### run - load and run a program from a file
#### Synopsis
```
run <file> [ <args> ... ]
```
#### Details
Loads **_<file>_**, which must be a 32-bit big-endian m68k ELF executable (as produced by the standard program build), and runs it. The program receives **_<file>_** as `argv[0]`, followed by any **_<args>_**.

Each loadable segment must lie between $00040000, where programs are loaded, and the end of RAM, and must not overlap memory the monitor is using. The whole file is checked before anything is loaded, so a bad file leaves memory unchanged. The program's memory is then kept for it until another program is loaded, so nothing the program (or the monitor) allocates can land on it.

The program runs on a stack of its own, 64KB unless it was linked with `-z stack-size=<bytes>` (the BIOS calls it makes use it too, needing up to about 5KB). If the program uses all, or nearly all, of it, a warning is printed when it ends.

After a successful **_run_**, **_go_** on its own runs the same program again without reloading it. Its data is not reset, so reload it with **_run_** if it depends on its initial values.

### ser1 | ser2
#### Synopsis
```
```

## BIOS calls

Programs run by the monitor use its services (the serial ports, files, memory and the clock) through BIOS calls. There are 29 of them, numbered from 0, and the numbers are defined in `bios.h` as `BIOS_PUTCHAR` and so on. Programs built with libmega rarely make them directly: the library functions do it for them (`open()` makes **BIOS_OPEN**, `malloc()` makes **BIOS_MALLOC**, and so on), and each call below names the library functions that use it.

### Making a call
A call is a `trap #0`, with its number and up to three arguments, each a long word, on the stack. From C, use the library's `syscall()`:

```
#include <bios.h>
#include <unistd.h>

int res = syscall(BIOS_TICKS, 0, 0, 0);
```

`syscall()` calls `do_trap0(number, arg1, arg2, arg3, &err)`, which is in the program startup code (`crt0`) and is just `trap #0` followed by `rts`. In assembler, do the same: push a pointer to a long word for the error number, then the three arguments (unused ones can be anything) and the call number, and `jsr do_trap0`. The trap handler expects that return address on the stack, between the exception frame and the call number, so `trap #0` can't be used inline.

```
        pea     err_num                 ; where the error number goes
        clr.l   -(sp)                   ; argument 3
        clr.l   -(sp)                   ; argument 2
        clr.l   -(sp)                   ; argument 1
        move.l  #4,-(sp)                ; the call number: BIOS_TICKS
        jsr     do_trap0
        lea     20(sp),sp
        ; d0 holds the result
```

The result comes back in `d0`. The trap handler saves and restores `d1`–`d7` and `a0`–`a6`, so only `d0` changes.

### Results and errors
Most calls return 0 (`OK`) when they work and -1 (`NOT_OK`) when they fail. Calls that return a count, a position or a pointer say so below. When a call fails, the monitor's `errno` value is stored in the long word given for it, and `syscall()` copies it into the program's `errno`. Check `errno` only after a call has failed: when one works, it can still hold the value from an earlier failure. A call number of 29 or more returns -1 and doesn't set the error number.

### Conventions
- **Serial ports** are numbered: 0 for SER1, the console, and 1 for SER2. Any other number fails. Opened as files, they are `//ser1` and `//ser2`.
- **Files**: a program starts with file descriptors 0 and 1 (standard input and output) open on SER1, and 2 (standard error) on SER2. Opening a file gives it the lowest free descriptor, and up to 32 can be open at once, including those three. Files the program leaves open are closed when it ends.
- **Paths** are like the monitor's: `/<mount>/...` from the top, such as `/cf0/bin/nethack`, or relative to the current directory. The current directory is shared with the monitor, so it's still in effect after the program ends.
- **Owners**: the memory calls take the caller's process ID, which is what the library's `getpid()` returns: 2 for a program, 1 for the monitor. A program's memory is freed when it ends.

### The calls

| Number | Name | Arguments | Returns |
|---|---|---|---|
| 0 | BIOS_PUTCHAR | port, character | the character |
| 1 | BIOS_GETCHAR | port | a character |
| 2 | BIOS_CHAR_AVAILABLE | port | 1 or 0 |
| 3 | BIOS_EXIT | status, pid | doesn't return |
| 4 | BIOS_TICKS | | milliseconds |
| 5 | BIOS_OPEN | path, flags, mode | a file descriptor |
| 6 | BIOS_CLOSE | fd | OK |
| 7 | BIOS_CREAT | path, mode | a file descriptor |
| 8 | BIOS_READ | fd, buffer, count | bytes read |
| 9 | BIOS_WRITE | fd, buffer, count | bytes written |
| 10 | BIOS_RESET_TICKS | | 0 |
| 11 | BIOS_CHDIR | path | OK |
| 12 | BIOS_GETCWD | buffer, size | the buffer |
| 13 | BIOS_MALLOC | size, pid | a pointer |
| 14 | BIOS_FREE | pointer, pid | nothing |
| 15 | BIOS_GETDENTS | fd, buffer, size | bytes stored |
| 16 | BIOS_TEST | three numbers | 42 |
| 17 | BIOS_SEEK | fd, offset, whence | the new position |
| 18 | BIOS_ISATTY | fd | 1 or 0 |
| 19 | BIOS_STAT | path, struct stat | OK |
| 20 | BIOS_FSTAT | fd, struct stat | OK |
| 21 | BIOS_FLUSH | port | 0 |
| 22 | BIOS_RAM_BANK | bank, base, size | OK |
| 23 | BIOS_REALLOC | pointer, size, pid | a pointer |
| 24 | BIOS_TIME | time | seconds since 1970 |
| 25 | BIOS_UNLINK | path | OK |
| 26 | BIOS_RMDIR | path | OK |
| 27 | BIOS_CHMOD | path, mode | OK |
| 28 | BIOS_MKDIR | path, mode | OK |

#### 0: BIOS_PUTCHAR - send a character
`int putchar(int port, int c)`

Sends **c** to serial **port**, waiting if its output buffer is full, and returns it. Nothing is added: the library's `putchar()` sends `\r` before each `\n` itself.

#### 1: BIOS_GETCHAR - read a character
`int getchar(int port)`

Waits for a character from serial **port** and returns it. Used by `getchar()`.

#### 2: BIOS_CHAR_AVAILABLE - check for input
`int char_available(int port)`

Returns 1 if a character from serial **port** is waiting to be read, otherwise 0. It doesn't wait. Used by the library's `char_available()`.

#### 3: BIOS_EXIT - end the program
`void exit(int status, pid_t pid)`

Frees the memory owned by **pid**, closes the files the program left open, and goes back to the monitor. It doesn't return. The library's `exit()` first runs the `atexit()` functions and the C++ destructors, and the program's startup code calls `exit()` with `main()`'s return value.

#### 4: BIOS_TICKS - read the millisecond counter
`uint32_t ticks(void)`

Returns the number of milliseconds since the monitor started, or since the last **BIOS_RESET_TICKS**. It goes back to 0 after about 49 days. Used by `ticks()`, `sleep()` and `usleep()`.

#### 5: BIOS_OPEN - open a file
`int open(const char *path, int flags, mode_t mode)`

Opens **path** and returns a file descriptor. **flags** is made up of these bits, from `fcntl.h`. Their values differ from Unix: `O_RDONLY` is 1, not 0, and must be given to read.

| Flag | Value | Meaning |
|---|---|---|
| O_RDONLY | $001 | open for reading |
| O_WRONLY | $002 | open for writing |
| O_RDWR | $003 | both |
| O_CREAT | $008 | create the file if it isn't there |
| O_DIRECTORY | $010 | it must be a directory (to read with **BIOS_GETDENTS**) |
| O_TRUNC | $040 | empty the file first |
| O_APPEND | $080 | write at the end of the file |
| O_EXCL | $100 | with O_CREAT: fail (EEXIST) if the file is there |

**mode** gives the permissions of a file that `O_CREAT` creates. The filesystem must be writable to create or write a file, so not the ROM disk. Used by `open()`, `fopen()` and `opendir()`.

#### 6: BIOS_CLOSE - close a file
`int close(int fd)`

Closes **fd**, writing out anything still buffered. Closing a serial port waits until what was written to it has been sent.

#### 7: BIOS_CREAT - create a file
`int creat(const char *path, mode_t mode)`

The same as **BIOS_OPEN** with `O_CREAT | O_WRONLY | O_TRUNC`.

#### 8: BIOS_READ - read from a file
`int read(int fd, char *buffer, size_t count)`

Reads up to **count** bytes into **buffer** and returns how many it read, 0 at the end of the file, or -1. It can return fewer bytes than asked for, even in the middle of a file, as it doesn't read past the end of the 2KB block it's in: call it again for the rest. From a serial port, it waits for the first character, then takes whatever else has already arrived, unchanged. A directory can't be read: use **BIOS_GETDENTS**.

#### 9: BIOS_WRITE - write to a file
`int write(int fd, const char *buffer, size_t count)`

Writes **count** bytes from **buffer** and returns how many it wrote, or -1. A file opened with `O_APPEND` is written at its end. On a serial port, each `\n` is sent as `\r\n`.

#### 10: BIOS_RESET_TICKS - reset the millisecond counter
`uint32_t reset_ticks(void)`

Sets the counter that **BIOS_TICKS** reads to 0, and returns 0.

#### 11: BIOS_CHDIR - change directory
`int chdir(const char *path)`

Makes **path** the current directory, for the program and for the monitor.

#### 12: BIOS_GETCWD - get the current directory
`char *getcwd(char *buffer, size_t size)`

Stores the current directory's path in **buffer**, which has room for **size** bytes, and returns **buffer**.

#### 13: BIOS_MALLOC - allocate memory
`void *malloc(size_t size, pid_t pid)`

Allocates **size** bytes, rounded up to a whole number of long words, for **pid**, and returns a pointer to them, or 0 (ENOMEM) if there isn't enough memory. The memory isn't cleared. It comes from bank 1 if that's fitted, then the top of bank 0, and it's freed when the program ends. A **size** of 0 prints the heap on the console (for debugging) and returns 0. Used by `malloc()`, `calloc()` and C++'s `new`.

#### 14: BIOS_FREE - free memory
`void free(void *pointer, pid_t pid)`

Frees memory from **BIOS_MALLOC** or **BIOS_REALLOC**. Memory that isn't owned by **pid** isn't freed (EPERM), and a pointer that isn't to an allocation is ignored.

#### 15: BIOS_GETDENTS - read a directory
`ssize_t getdents(int fd, struct dirent *buffer, size_t size)`

Reads entries from a directory opened with **BIOS_OPEN**, into **buffer**, which has room for **size** bytes. Each entry is a whole `struct dirent` (from `dirent.h`), so **size** must be at least one of those. Returns the number of bytes stored, a multiple of `sizeof(struct dirent)`, 0 when there are no more, or -1. Used by `readdir()`.

```
struct dirent {
    ino_t           d_ino;          // inode number
    off_t           d_off;
    unsigned short  d_reclen;
    unsigned char   d_type;         // DT_REG, DT_DIR, DT_LNK, ...
    char            d_name[256];
};
```

#### 16: BIOS_TEST - test the BIOS call mechanism
`int test(int p1, int p2, int p3)`

Prints its three arguments, in hex, on the console, and returns 42.

#### 17: BIOS_SEEK - move in a file
`off_t lseek(int fd, off_t offset, int whence)`

Moves the position in **fd** to **offset** bytes from the start (`SEEK_SET`, 0), the current position (`SEEK_CUR`, 1) or the end (`SEEK_END`, 2), and returns the new position. A serial port can't seek (ESPIPE). Used by `lseek()`, `fseek()` and `ftell()`.

#### 18: BIOS_ISATTY - is it a terminal?
`int isatty(int fd)`

Returns 1 if **fd** is a serial port, otherwise 0 (ENOTTY).

#### 19: BIOS_STAT - get a file's details
`int stat(const char *path, struct stat *st)`

Fills in **st** (from `sys/stat.h`) for **path**, without opening it.

```
struct stat {
    dev_t       st_dev;         // which mount the file is on
    ino_t       st_ino;         // inode number (0 if the filesystem has none)
    mode_t      st_mode;        // file type (S_IFREG, S_IFDIR, ...) and permissions
    nlink_t     st_nlink;       // number of hard links
    uid_t       st_uid;
    gid_t       st_gid;
    off_t       st_size;        // size in bytes
    time_t      st_atime;       // last access
    time_t      st_mtime;       // last modification
    time_t      st_ctime;       // last status change
    blksize_t   st_blksize;     // best size for reads and writes
    blkcnt_t    st_blocks;      // space used, in 512-byte units
};
```

#### 20: BIOS_FSTAT - get an open file's details
`int fstat(int fd, struct stat *st)`

The same as **BIOS_STAT**, for an open file. For a serial port, only `st_mode` (`S_IFCHR`, permissions 666) and `st_nlink` are filled in.

#### 21: BIOS_FLUSH - wait for output to be sent
`int flush(int port)`

Waits until everything written to serial **port** has been sent, and returns 0. Used by `fflush()`, for SER1.

#### 22: BIOS_RAM_BANK - find a RAM bank
`int ram_bank(int bank, uint32_t *base, uint32_t *size)`

Stores where RAM **bank** (0 or 1) starts in **\*base**, and how much usable RAM it has in **\*size**. A bank that isn't fitted, or that failed its test at boot, has a size of 0. Any other bank number fails (EINVAL).

#### 23: BIOS_REALLOC - resize memory
`void *realloc(void *pointer, size_t size, pid_t pid)`

Resizes an allocation to **size** bytes, keeping its contents up to the smaller of the two sizes, and returns where it now is, which may have moved. A **pointer** of 0 is the same as **BIOS_MALLOC**, and a **size** of 0 frees it and returns 0. If there isn't enough memory, it returns 0 and the old allocation is left as it was.

#### 24: BIOS_TIME - get the time
`time_t time(time_t *t)`

Returns the time from the real-time clock, in seconds since 1 January 1970, and stores it in **\*t** too, unless **t** is 0. Without a clock, or if it doesn't hold a sensible time, it returns 0. Used by `time()`.

#### 25: BIOS_UNLINK - delete a file
`int unlink(const char *path)`

Deletes **path**, which can't be a directory (EISDIR) or a file that's open (EBUSY). If the file has other names (hard links), only this one goes. Used by `unlink()` and `remove()`.

#### 26: BIOS_RMDIR - delete a directory
`int rmdir(const char *path)`

Deletes the directory **path**, which must be empty (ENOTEMPTY). The current directory, and the top directory of a mount, can't be deleted. Used by `rmdir()` and `remove()`.

#### 27: BIOS_CHMOD - set permissions
`int chmod(const char *path, mode_t mode)`

Sets the permissions of **path** to the low 12 bits of **mode** (the read, write and execute bits, setuid, setgid and sticky).

#### 28: BIOS_MKDIR - make a directory
`int mkdir(const char *path, mode_t mode)`

Makes the directory **path**, with permissions **mode**. The directory it goes in must already exist (ENOENT), and **path** mustn't (EEXIST).

## Installing the monitor

