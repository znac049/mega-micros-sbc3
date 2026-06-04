LIB=stdlib
LIBOBJECTS=$(DIR)/atoi.o \
			$(DIR)/atol.o \
			$(DIR)/asctime.o \
			$(DIR)/chdir.o \
			$(DIR)/close.o \
			$(DIR)/ctime.o \
			$(DIR)/err.o \
			$(DIR)/errno.o \
			$(DIR)/fclose.o \
			$(DIR)/fflush.o \
			$(DIR)/fgetc.o \
			$(DIR)/fgets.o \
			$(DIR)/fprintf.o \
			$(DIR)/fputc.o \
			$(DIR)/fputs.o \
			$(DIR)/fread.o \
			$(DIR)/fwrite.o \
			$(DIR)/getchar.o \
			$(DIR)/getenv.o \
			$(DIR)/gets.o \
			$(DIR)/gmtime.o \
			$(DIR)/isatty.o \
			$(DIR)/isdigit.o \
			$(DIR)/isspace.o \
			$(DIR)/itoa.o \
			$(DIR)/localtime.o \
			$(DIR)/malloc.o \
			$(DIR)/memcpy.o \
			$(DIR)/memset.o \
			$(DIR)/mktime.o \
			$(DIR)/open.o \
			$(DIR)/printf.o \
			$(DIR)/putchar.o \
			$(DIR)/puts.o \
			$(DIR)/rand.o \
			$(DIR)/read.o \
			$(DIR)/snprintf.o \
			$(DIR)/srand.o \
			$(DIR)/sscanf.o \
			$(DIR)/strchr.o \
			$(DIR)/strcasecmp.o \
			$(DIR)/strcmp.o \
			$(DIR)/strcpy.o \
			$(DIR)/streams.o \
			$(DIR)/strlen.o \
			$(DIR)/strncpy.o \
			$(DIR)/strrchr.o \
			$(DIR)/strtol.o \
			$(DIR)/tolower.o \
			$(DIR)/toupper.o \
			$(DIR)/vfprintf.o \
			$(DIR)/vsnprintf.o \
			$(DIR)/vsscanf.o \
			$(DIR)/warn.o \
			$(DIR)/write.o

LIBINCLUDES=$(DIR)/include

# ---===---
DIR := $(shell dirname $(lastword $(MAKEFILE_LIST)))
BINARY := lib$(LIB).a
CFLAGS  := $(CFLAGS) -I$(LIBINCLUDES) 
OBJECTS := $(OBJECTS) $(LIBOBJECTS)
INCLUDES := $(INCLUDES) $(DIR)/include/*
LIBS := $(LIBS) $(DIR)/$(BINARY)

$(DIR)/$(BINARY): $(LIBOBJECTS)
	$(AR) $(ARFLAGS) rs $@ $^
	$(RANLIB) $@
