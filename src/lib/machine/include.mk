LIBOBJECTS=\
			bios_call.o \
			_cf.o \
			cf.o \
			cpu.o \
			disk.o \
			duart2.o \
			filesystems.o \
			i2c.o \
			leds.o \
			_misc.o \
			misc.o \
			pit.o \
			pre_main.o \
			safeio.o \
			_traps.o \
			_vectors.o \
			vectors.o

LIBINCLUDES=$(DIR)/include

DIR := $(shell dirname $(lastword $(MAKEFILE_LIST)))
CFLAGS  := $(CFLAGS) -I$(LIBINCLUDES)
OBJECTS := $(OBJECTS) $(LIBOBJECTS)
INCLUDES := $(INCLUDES) $(DIR)/include/*

VPATH += machine/ext2fs

include $(DIR)/ext2fs/include.mk
