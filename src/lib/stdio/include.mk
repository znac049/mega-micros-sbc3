LIBOBJECTS=clearerr.o \
			perror.o \
			scan.o \
			setbuf.o \
			ungetc.o \
			fclose.o \
			feof.o \
			ferror.o \
			fflush.o \
			fgetc.o \
			fgetpos.o \
			fgets.o \
			fileno.o \
			fopen.o \
			fprintf.o \
			fputc.o \
			fputs.o \
			fscanf.o \
			fread.o \
			fseek.o \
			fsetpos.o \
			ftell.o \
			fwrite.o \
			getchar.o \
			gets.o \
			printf.o \
			putchar.o \
			puts.o \
			remove.o \
			rewind.o \
			snprintf.o \
			sprintf.o \
			sscanf.o \
			vfprintf.o \
			vprintf.o \
			vsnprintf.o \
			vsprintf.o \
			vsscanf.o


LIBINCLUDES=$(DIR)/include

DIR := $(shell dirname $(lastword $(MAKEFILE_LIST)))
CFLAGS  := $(CFLAGS) -I$(LIBINCLUDES) 
OBJECTS := $(OBJECTS) $(LIBOBJECTS)
#INCLUDES := $(INCLUDES) $(DIR)/include/*
