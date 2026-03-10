PROJECT = bob4

BASE_ADDRESS  = 0x280000

MAKEFLAGS += -rR

PFX=m68k-linux-gnu
CC=$(PFX)-gcc -S -mcpu=68030
AS=$(PFX)-gcc -c -mcpu=68030
LD=$(PFX)-ld
OBJCOPY=$(PFX)-objcopy
OBJDUMP=$(PFX)-objdump

CFLAGS=-O0 -fpic
ASFLAGS=
LDFLAGS=-O0 -Bstatic --strip-all -Ttext=$(BASE_ADDRESS) -z noexecstack

CSRCS=$(wildcard *.c)
COBJS=$(subst .c,.o,$(CSRCS))

ASMSRCS=$(wildcard *.S)
ASMOBJS=$(subst .S,.o,$(ASMSRCS))

OBJS=$(ASMOBJS) $(COBJS)

.SUFFIXES: .o .s .c .S
.PHONY: all clean

.c.o:
		$(CC) $(CFLAGS) $<
		$(AS) $(ASFLAGS) $*.s -o $@

.S.o:
		$(AS) $(ASFLAGS) $<


all: $(OBJS)
		$(LD) $(LDFLAGS) $(OBJS) -o $(PROJECT).out -Map=$(PROJECT).map --init=_start
		$(OBJCOPY) -O srec $(PROJECT).out $(PROJECT).srec
		$(OBJCOPY) -O binary $(PROJECT).out $(PROJECT).bin
		$(OBJDUMP) --disassemble $(PROJECT).out

clean:
		rm -f *.out *.o *.bin *.srec *.s *.map *~