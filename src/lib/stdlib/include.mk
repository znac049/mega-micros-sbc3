#LIB=stdlib
LIBOBJECTS=atoi.o \
			atol.o \
			calloc.o \
			exit.o \
			free.o \
			getenv.o \
			itoa.o \
			malloc.o \
			printf.o \
			putchar.o \
			puts.o \
			rand.o \
			read.o \
			realpath.o \
			snprintf.o \
			srand.o \
			strtol.o

LIBINCLUDES=$(DIR)/include

DIR := $(shell dirname $(lastword $(MAKEFILE_LIST)))
CFLAGS  := $(CFLAGS) -I$(LIBINCLUDES) 
OBJECTS := $(OBJECTS) $(LIBOBJECTS)
#INCLUDES := $(INCLUDES) $(DIR)/include/*
