LIBOBJECTS=chdir.o \
			close.o \
			getcwd.o \
			getdents.o \
			getpid.o \
			isatty.o \
			mktime.o \
			read.o \
			syscall.o \
			write.o

LIBINCLUDES=$(DIR)/include

DIR := $(shell dirname $(lastword $(MAKEFILE_LIST)))
CFLAGS  := $(CFLAGS) -I$(LIBINCLUDES) 
OBJECTS := $(OBJECTS) $(LIBOBJECTS)
#INCLUDES := $(INCLUDES) $(DIR)/include/*
