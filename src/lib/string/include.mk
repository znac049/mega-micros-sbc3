LIBOBJECTS=memcpy.o \
			memset.o \
			strcat.o \
			strchr.o \
			strcmp.o \
			strcpy.o \
			strlen.o \
			strncat.o \
			strncmp.o \
			strncpy.o \
			strrchr.o

LIBINCLUDES=$(DIR)/include

DIR := $(shell dirname $(lastword $(MAKEFILE_LIST)))
CFLAGS  := $(CFLAGS) -I$(LIBINCLUDES) 
OBJECTS := $(OBJECTS) $(LIBOBJECTS)
#INCLUDES := $(INCLUDES) $(DIR)/include/*
