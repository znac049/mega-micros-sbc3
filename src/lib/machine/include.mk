LIBOBJECTS=\
			block_devices.o \
			_cf.o \
			cf.o \
			cpu.o \
			disk.o \
			ds1307.o \
			duart.o \
			filesystems.o \
			i2c.o \
			leds.o \
			_misc.o \
			misc.o \
			pit.o \
			pre_main.o \
			safeio.o \
			sh1107.o \
			_traps.o \
			_vectors.o \
			vectors.o

LIBINCLUDES=$(DIR)/include

DIR := $(shell dirname $(lastword $(MAKEFILE_LIST)))
CFLAGS  := $(CFLAGS) -I$(LIBINCLUDES)
OBJECTS := $(OBJECTS) $(LIBOBJECTS)
INCLUDES := $(INCLUDES) $(DIR)/include/*

VPATH += machine/ext2fs machine/block_devices

include $(DIR)/ext2fs/include.mk
include $(DIR)/block_devices/include.mk
