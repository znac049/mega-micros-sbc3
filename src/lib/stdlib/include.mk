LIB=stdlib
LIBOBJECTS=$(DIR)/atoi.o \
			$(DIR)/atol.o \
			$(DIR)/err.o \
			$(DIR)/fclose.o \
			$(DIR)/fflush.o \
			$(DIR)/fgetc.o \
			$(DIR)/fgets.o \
			$(DIR)/fputc.o \
			$(DIR)/fputs.o \
			$(DIR)/fread.o \
			$(DIR)/fwrite.o \
			$(DIR)/getchar.o \
			$(DIR)/getenv.o \
			$(DIR)/gets.o \
			$(DIR)/isdigit.o \
			$(DIR)/itoa.o \
			$(DIR)/memset.o \
			$(DIR)/printf.o \
			$(DIR)/putchar.o \
			$(DIR)/puts.o \
			$(DIR)/rand.o \
			$(DIR)/srand.o \
			$(DIR)/sscanf.o \
			$(DIR)/strchr.o \
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
			$(DIR)/warn.o

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
