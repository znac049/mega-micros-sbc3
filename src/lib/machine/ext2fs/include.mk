LIBOBJECTS := $(LIBOBJECTS) \
			blocks.o \
			dir.o \
			dump.o \
			endian.o \
			ext2.o \
			file.o

LIBINCLUDES=$(DIR)/include

DIR := $(shell dirname $(lastword $(MAKEFILE_LIST)))
CFLAGS  := $(CFLAGS) -I$(LIBINCLUDES)
OBJECTS := $(OBJECTS) $(LIBOBJECTS)
