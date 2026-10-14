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
- A C runtime library is included which provided a comprehensive (and getting more complete every day) set of many of the standard functions you'd expect to find on any linux or unix system.
- ROM validity checked on reset.
- Memory test.

## Monitor Commands

### help
Simply prints a brief summary of each command the monitor accepts
```
/# help
Commands are:
  cat <filename>
  cd <path>
  dir [ <path> ]
  disassemble <address>
  dump [<start_address> [<count>]]
  eval <expression>
  go <address>]
  load
  probe
  rtc (erase) | (time [hh:mm[:ss]]) | (date [yyyy:mm:dd])
  ser1|2 baud <baudrate>
  quit

/#
```

### byteset, wordset, longset
Edit data in memory.
__Aliases__: **bs**, **ws**, **ls**

```
bs [ = <expr> [, <expr> ...]]
bs <address> [ [ = ] <expr> [, <expr> ...]]
@[b|w|l] [ <address> ] [ [ = ] <expr> [, <expr> ...]]
```

### cat - Display the contents of a file
#### Synopsis
```
cat <filename>
``` 
#### Details
The file specified by **_<filename>_** is displayed on the console. If **_<filename>_** does not contain a path part, it will be read from the current directory.

### cd - Change directory

### dir - List the contents of a directory

### disassemble - disassemble machine code

### dump - dump contents of memory

### eval - Evaluate an expression
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

### pwd

### rtc

### ser1 | ser2

## Installing the monitor

### Booting
#### Boot errors
How boot errors are displayed depends on how early in the initialisation code they occur. If an error occurs before the duart has been initialise, LEDs will be used to indicate what has occurred. If the system has initialised the duart, it will display an appropriate error message on the console serial port.
