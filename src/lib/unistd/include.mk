LIBOBJECTS=access.o \
			chdir.o \
			close.o \
			getcwd.o \
			getdents.o \
			getpid.o \
			getuid.o \
			isatty.o \
			lseek.o \
			mktime.o \
			read.o \
			rmdir.o \
			sleep.o \
			syscall.o \
			unlink.o \
			write.o

LIBINCLUDES=$(DIR)/include

DIR := $(shell dirname $(lastword $(MAKEFILE_LIST)))
CFLAGS  := $(CFLAGS) -I$(LIBINCLUDES) 
OBJECTS := $(OBJECTS) $(LIBOBJECTS)
#INCLUDES := $(INCLUDES) $(DIR)/include/*
