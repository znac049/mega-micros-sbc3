LIBOBJECTS := $(LIBOBJECTS) \
			e2blocks.o \
			e2dir.o \
			e2dump.o \
			e2endian.o \
			ext2.o \
			e2file.o \
			e2search.o

LIBINCLUDES=$(SUBDIR)/include

SUBDIR := $(shell dirname $(lastword $(MAKEFILE_LIST)))
CFLAGS  := $(CFLAGS) -I$(LIBINCLUDES)
OBJECTS := $(OBJECTS) $(LIBOBJECTS)
