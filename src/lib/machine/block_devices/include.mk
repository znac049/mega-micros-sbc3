LIBOBJECTS := $(LIBOBJECTS) \
			cf_block.o \
			rom_block.o

LIBINCLUDES=$(SUBDIR)/include

SUBDIR := $(shell dirname $(lastword $(MAKEFILE_LIST)))
CFLAGS  := $(CFLAGS) -I$(LIBINCLUDES)
OBJECTS := $(OBJECTS) $(LIBOBJECTS)
