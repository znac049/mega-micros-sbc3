LIBOBJECTS=fclose.o \
			fflush.o \
			fgetc.o \
			fgets.o \
			fopen.o \
			fprintf.o \
			fputc.o \
			fputs.o \
			fread.o \
			fwrite.o \
			getchar.o \
			gets.o \
			printf.o \
			putchar.o \
			puts.o \
			snprintf.o \
			sscanf.o \
			vfprintf.o \
			vsnprintf.o \
			vsscanf.o


LIBINCLUDES=$(DIR)/include

DIR := $(shell dirname $(lastword $(MAKEFILE_LIST)))
CFLAGS  := $(CFLAGS) -I$(LIBINCLUDES) 
OBJECTS := $(OBJECTS) $(LIBOBJECTS)
#INCLUDES := $(INCLUDES) $(DIR)/include/*
