# MicroMon User Guide
This manual provides full documentation for the MicoMon monitor for the [Mega-Micros 68030 SBC-3 board](https://mega-micros.co.uk/index_68030.htm).

MicroMon is an alternative monitor to the one supplied with the board, which better suited my needs and also gave me the opportunity to really get to grips with the board and it's hardware.

It is written predominantly in C with a minimal amount of assembly language.

## Features
- Takes care of all low-level initialisation of the board.
  - Detects the amount of memory
  - Detects which duart variant is being used and if it is being overclocked.
- Allows memory to be inspected
- Allows programs to be loaded and then run via serial port, in [Motorola S-Record format](https://en.wikipedia.org/wiki/Motorola_S-record)
- Provides a number of bios/system calls from the user program to take care of most I/O:
  - Both serial ports
  - LED control
  - Access to the onboard DS-1307 real time clock.
  - Compact Flash (ext2 filesystem)
  - ROM based disk (ext2 filesystem)
- A C runtime library is included which provided a comprehensive (and getting more complete every day) set of many of the standard functions you'd expect to find on any linux or unix system.

## Monitor Commands

### help
Simply prints a brief summary of each command the monitor accepts

### cat

### cd

### dir

### disassemble

### dump

### eval

### go

### load

### probe

### pwd

### rtc

### ser1 | ser2

## Installing the monitor

### Booting
#### Boot errors
How boot errors are displayed depends on how early in the initialisation code they occur. If an error occurs before the duart has been initialise, LEDs will be used to indicate what has occurred. If the system has initialised the duart, it will display an appropriate error message on the console serial port.
