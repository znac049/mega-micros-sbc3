#!/bin/bash
echo "## Build Script V1.0 - building example1.s ##"
m68k-linux-gnu-gcc -c example1.s
m68k-linux-gnu-ld -Ttext=0x100000 example1.o
m68k-linux-gnu-objcopy -O binary a.out example1.bin
m68k-linux-gnu-objcopy -O srec a.out example1.srec
echo "## Build Script - done! ##"
ls
