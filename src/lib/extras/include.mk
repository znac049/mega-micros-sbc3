LIBOBJECTS=char_available.o \
			dump_mem.o \
			ltrim.o \
			midstr.o \
			reset_ticks.o \
			rtrim.o \
			split_str.o \
			strblank.o \
			strpad.o \
			ticks.o

LIBINCLUDES=$(DIR)/include

DIR := $(shell dirname $(lastword $(MAKEFILE_LIST)))
CFLAGS  := $(CFLAGS) -I$(LIBINCLUDES)
OBJECTS := $(OBJECTS) $(LIBOBJECTS)
#INCLUDES := $(INCLUDES) $(DIR)/include/*
