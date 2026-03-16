ifndef MEGA_MICROS_DIR
$(error ERROR: MEGA_MICROS_DIR not set)
endif

CPU?=68030
ARCH?=$(CPU)
TUNE?=$(CPU)

SYSINCDIR?=$(MEGA_MICROS_DIR)/built/include
SYSLIBDIR?=$(MEGA_MICROS_DIR)/built/lib

VASMFLAGS=-Felf -m$(CPU) -quiet -Lnf $(DEFINES)
VASM=vasmm68k_mot

CP=cp

OBJ=crt0.o

%.o : %.asm
	$(VASM) $(VASMFLAGS) $(EXTRA_VASMFLAGS) -L $(basename $@).lst -o $@ $<

.PHONY: all install clean

all: $(OBJ)

install: all
	$(CP) $(OBJ) $(MEGA_MICROS_DIR)/built/lib

clean:
	rm -f *.o *~