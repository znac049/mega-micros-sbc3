ifndef MEGA_MICROS_DIR
$(error MEGA_MICROS_DIR not set)
endif

-include $(MEGA_MICROS_DIR)/src/stdenv.mk


#.PHONY: all clean#
#
#PROG = hello
#BASE_ADDRESS  = 0x100000

#MAKEFLAGS += -rR
#PFX=m68k-linux-gnu
#CC=$(PFX)-gcc -c -mcpu=68030
#AS=$(PFX)-gcc -c -mcpu=68030
#LD=$(PFX)-ld
#OBJCOPY=$(PFX)-objcopy

#CFLAGS=-O2
#ASFLAGS=
#LDFLAGS=-Bstatic --strip-all -Ttext=$(BASE_ADDRESS) -z noexecstack --init=_start
#LIBS=-L../lib/libs -lc -ltutor 

#CSRCS=$(wildcard *.c)
#COBJS=$(subst .c,.o,$(CSRCS))
#ASMSRCS=$(wildcard *.S)
#ASMOBJS=$(subst .S,.o,$(ASMSRCS))

#OBJS=$(COBJS) $(ASMOBJS)

#.SUFFIXES: .o .c .S
#.c.o:
#	$(CC) $(CFLAGS) $<
#.S.o:
#	$(AS) $(ASFLAGS) $<

#all: $(PROG)

#$(PROG): $(OBJS)#
#	$(LD) $(LDFLAGS) $(OBJS) -o $(PROG).out -Map=$(PROG).map $(LIBS)#
#	$(OBJCOPY) -O srec $(PROG).out $(PROG).srec
#	$(OBJCOPY) -O binary $(PROG).out $(PROG).bin

#clean:
#	rm -f *.out *.o *.bin *.srec *.s *.map *~