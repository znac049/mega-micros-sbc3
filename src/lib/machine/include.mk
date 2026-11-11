LIBOBJECTS=\
			_traps.o \
			vectors.o

LIBINCLUDES=$(DIR)/include

DIR := $(shell dirname $(lastword $(MAKEFILE_LIST)))
CFLAGS  := $(CFLAGS) -I$(LIBINCLUDES)
OBJECTS := $(OBJECTS) $(LIBOBJECTS)
INCLUDES := $(INCLUDES) $(DIR)/include/*

VPATH += machine/ext2fs machine/block_devices
