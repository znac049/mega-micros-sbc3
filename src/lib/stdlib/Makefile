MAKEFLAGS += -rR

CSRCS=$(wildcard *.c)
COBJS=$(subst .c,.o,$(CSRCS))
OBJS=$(COBJS) $(ASMOBJS)

ASMSRCS=$(wildcard *.S)
ASMOBJS=$(subst .S,.o,$(ASMSRCS))

INCLUDE_DIR=/home/bob/src/mega-micros/lib/includes
HDR_FILES=assert.h float.h math.h stdarg.h stdlib.h ctype.h \
	limits.h setjmp.h stddef.h string.h errno.h locale.h \
	signal.h stdio.h time.h

LIB_DIR=/home/bob/src/mega-micros/lib/libs

PFX=m68k-linux-gnu
CC=$(PFX)-gcc -mcpu=68030 -nostdinc -I /home/bob/src/mega-micros/lib/includes --sysroot=/home/bob/src/mega-micros/lib
AS=$(PFX)-gcc -c -mcpu=68030
AR=$(PFX)-ar

CFLAGS=-O2 -c
ASFLAGS=

.SUFFIXES:
.SUFFIXES: .o .c .S

.c.o:
	$(CC) $(CFLAGS) $<

.S.o:
	$(AS) $(ASFLAGS) $<

.PHONY: all clean install

all: libc.a

proto.h: $(CSRCS)
	cproto $(CSRCS) >proto.h

$(COBJS): proto.h $(HDR_FILES)

libc.a: $(OBJS)
	$(AR) r $@ $(OBJS)

install: libc.a
	mkdir -p $(INCLUDE_DIR)
	cp $(HDR_FILES) $(INCLUDE_DIR)
	cp libc.a $(LIB_DIR)

clean:
	rm -f *.o *.a


