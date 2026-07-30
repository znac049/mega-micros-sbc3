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

I do all my development under Linux so you migyht be able to get a build environment setup under a different OS, but the following instructions assume Linux. Of course, if you manage to do it on a different OS, I'd love it if you'd tell me how and I'll update these instructions.

## Installation (tested with Ubuntu 24.04)
- check out the repository to a location of your choice
- install a toolchain
- set a single environment variable
- start having fun!

### Installing the toolchain
#### gcc
```
$ sudo apt install gcc-m68k-linux-gnu
```

#### vasm
I despise the _gnu-as_ syntax and much prefer Motorola style syntax. For that reason, I use _vasm_ instead of _gnu-as_ for my assembly code. You have to download and build it from source code, but it's really not that hard:
```
$ mkdir tmp
$ cd tmp
$ wget http://phoenix.owl.de/tags/vasm2_0e.tar.gz
$ tar xvfz vasm2_0e.tar.gz
$ cd vasm
$ make CPU=m68k SYNTAX=mot
$ cp vasmm68k_mot <somewhere on your path>
$ cd ..
$ # Optional:
$ rm -rf tmp
```
> [!NOTE] 
> there may be newer versions of _vasm_ which should work just fine. At thge time of writing, I am using 2.0e

### Environment
All you need to do is add an environment variable called `MEGA_MICROS_DIR` which contains the full path to wherever you checked out the git repository. Foe example, on my system, where I use the bash shell, I put the following at the end of my `~/.bashrc` file:
```
# Mega-Micros 68k stuff
export MEGA_MICROS_DIR=/home/bob/src/mega-micros
```
Once that environment variable has been set, you should be good to start compiling stuff.

> [!NOTE]
> The supplied makefiles assume that the 68k compiler and _vasm_ can be found somewhere on your search path.

## First Steps
### Build the run-time libraries and crt0
If you've followed the instructions above, it's as simple as (start in the directory you git cloned into):
```
$ cd src/lib
$ make clean 
$ make
```