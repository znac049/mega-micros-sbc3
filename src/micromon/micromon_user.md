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

## Installing the monitor

