LIBOBJECTS := $(LIBOBJECTS) \
			e2blocks.o \
			e2dir.o \
			e2dump.o \
			e2endian.o \
			ext2.o \
			e2file.o

LIBINCLUDES=$(DIR)/include

DIR := $(shell dirname $(lastword $(MAKEFILE_LIST)))
CFLAGS  := $(CFLAGS) -I$(LIBINCLUDES)
OBJECTS := $(OBJECTS) $(LIBOBJECTS)
