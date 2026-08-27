LIBOBJECTS=closedir.o \
			dirfd.o \
			opendir.o \
			readdir.o \
			rewinddir.o \
			seekdir.o \
			telldir.o

LIBINCLUDES=$(DIR)/include

DIR := $(shell dirname $(lastword $(MAKEFILE_LIST)))
CFLAGS  := $(CFLAGS) -I$(LIBINCLUDES) 
OBJECTS := $(OBJECTS) $(LIBOBJECTS)
#INCLUDES := $(INCLUDES) $(DIR)/include/*
