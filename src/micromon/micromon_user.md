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
__Aliases__: **bs**, **ws**, **ls**

```
bs [ = <expr> [, <expr> ...]]
bs <address> [ [ = ] <expr> [, <expr> ...]]
@[b|w|l] [ <address> ] [ [ = ] <expr> [, <expr> ...]]
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

### dir - list the contents of a directory
#### Synopsis
```
```

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

### load - load a program from the console port.
#### Synopsis
```
load
```
#### Details
The monitor loads a program, in Motorola S-Record format, from the console port. A countdown is displayed and if no data has been received when it hits zero, the load is aborted. If a start address record is present, it will be remembered and used by the **_go_** command, if no other address is specified.

### probe
#### Synopsis
```
```

### pwd - print current directory
#### Synopsis
```
pwd
```

### rtc - configure/show clock settings
#### Synopsis
```
```

### ser1 | ser2
#### Synopsis
```
```

## Installing the monitor

## Monitor Error Codes
The following error codes may be displayed on the Hex Display card, if present:
| First Digit | Digits 3 and 4 | Meaning |
|:------------|:--------:|:--------------|
| 01 | 2000 | ROM Checksum failed. CPU stopped. |
