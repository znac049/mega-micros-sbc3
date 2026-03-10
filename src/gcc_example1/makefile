# simple makefile for the gcc assembler

PROJECT = example1
GOADDR  = 0x280000

all:
	#vasmm68k_mot -m68000 -Felf -L example1.lis -nocase -o example1.o example1.asm
	m68k-linux-gnu-gcc -c $(PROJECT).s -mcpu=68030
	m68k-linux-gnu-ld -Ttext=$(GOADDR) $(PROJECT).o -o $(PROJECT).out
	m68k-linux-gnu-objcopy -O srec $(PROJECT).out $(PROJECT).srec
	m68k-linux-gnu-objdump --disassemble example1.out

clean:
	rm -f *.out *.o *.bin *.srec *.lis *.elf
