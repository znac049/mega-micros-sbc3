#LIB=stdlib
LIBOBJECTS=atoi.o \
			atol.o \
			asctime.o \
			char_available.o \
			chdir.o \
			close.o \
			closedir.o \
			creat.o \
			ctime.o \
			err.o \
			errno.o \
			exit.o \
			fclose.o \
			fflush.o \
			fgetc.o \
			fgets.o \
			fprintf.o \
			fputc.o \
			fputs.o \
			fread.o \
			fwrite.o \
			getchar.o \
			getcwd.o \
			getenv.o \
			gets.o \
			gmtime.o \
			isalnum.o \
			isalpha.o \
			isatty.o \
			isblank.o \
			isdigit.o \
			isspace.o \
			isxdigit.o \
			itoa.o \
			localtime.o \
			malloc.o \
			memcpy.o \
			memset.o \
			mktime.o \
			open.o \
			opendir.o \
			printf.o \
			putchar.o \
			puts.o \
			rand.o \
			read.o \
			readdir.o \
			realpath.o \
			rewinddir.o \
			seekdir.o \
			setjmp.o \
			snprintf.o \
			srand.o \
			sscanf.o \
			strcat.o \
			strchr.o \
			strcasecmp.o \
			strncasecmp.o \
			strcmp.o \
			strcpy.o \
			streams.o \
			strlen.o \
			strncat.o \
			strncpy.o \
			strrchr.o \
			strtol.o \
			telldir.o \
			tolower.o \
			toupper.o \
			vfprintf.o \
			vsnprintf.o \
			vsscanf.o \
			warn.o \
			write.o

LIBINCLUDES=$(DIR)/include

DIR := $(shell dirname $(lastword $(MAKEFILE_LIST)))
CFLAGS  := $(CFLAGS) -I$(LIBINCLUDES) 
OBJECTS := $(OBJECTS) $(LIBOBJECTS)
INCLUDES := $(INCLUDES) $(DIR)/include/*
