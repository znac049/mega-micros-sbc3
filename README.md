# mega-micros-sbc3
Repo to capture my journey with the Mega-Micros 68030 SBC3

Included in this repo, you will find the following:
- Comprehensive set of libraries for the SBC3, including standard C runtime and code for accessing the hardware.
- Code for an alternative boot rom, giving:
  - built-in disassembler
  - load and run code via serial
  - memory examination commands
  - a built in expression evaluator which allows expressions to be entered wherever a number is expected
  - Access to bios functions via a system call mechanism (trap #0)
- Easy to setup build environment allows code to be targetted for bare metal or a simple DOS

## Installation
### Linux (tested with Ubuntu 24.04)
- check out the repository to a location of your choice
- install a toolchain
- set a single environment variable
- start having fun!

#### Installing the toolchain
##### gcc
```
$ sudo apt install gcc-m68k-linux-gnu
```

##### vasm
I despise the gnu-as syntax and much prefer Motorola style syntax. For that reason, I use vasm instead of gas for my assembly code. You have to download and built it from sourcecode, but it's really not that hard:
```
$ mkdir tmp
$ cd tmp
$ wget http://phoenix.owl.de/tags/vasm2_0e.tar.gz
$ tar xvfz vasm2_0e.tar.gz
$ cd vasm
$ make CPU=m68k SYNTAX=mot
$ cp vasmm68k_mot <somewhere on your path>
```
Note. there may be newer versions of vasm which should work just fine. At thge time of writing, I am using 2.0e
