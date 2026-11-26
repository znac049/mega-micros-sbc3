LIBOBJECTS=memcmp.o\
			memcpy.o \
			memmove.o \
			memset.o \
			strcat.o \
			strchr.o \
			strcmp.o \
			strcpy.o \
			strcspn.o \
			strdup.o \
			strerror.o \
			strlen.o \
			strncat.o \
			strncmp.o \
			strncpy.o \
			strpbrk.o \
			strrchr.o \
			strspn.o \
			strstr.o

LIBINCLUDES=$(DIR)/include

DIR := $(shell dirname $(lastword $(MAKEFILE_LIST)))
CFLAGS  := $(CFLAGS) -I$(LIBINCLUDES) 
OBJECTS := $(OBJECTS) $(LIBOBJECTS)
#INCLUDES := $(INCLUDES) $(DIR)/include/*
