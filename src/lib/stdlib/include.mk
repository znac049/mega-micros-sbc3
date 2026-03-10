LIB=stdlib
LIBOBJECTS=$(DIR)/atol.o \
			$(DIR)/err.o \
			$(DIR)/fclose.o \
			$(DIR)/fflush.o \
			$(DIR)/fgets.o \
			$(DIR)/fputc.o \
			$(DIR)/fputs.o \
			$(DIR)/fread.o \
			$(DIR)/fwrite.o \
			$(DIR)/getchar.o \
			$(DIR)/getenv.o \
			$(DIR)/gets.o \
			$(DIR)/memset.o \
			$(DIR)/printf.o \
			$(DIR)/putchar.o \
			$(DIR)/puts.o \
			$(DIR)/rand.o \
			$(DIR)/snprintf.o \
			$(DIR)/srand.o \
			$(DIR)/sscanf.o \
			$(DIR)/strcmp.o \
			$(DIR)/strcpy.o \
			$(DIR)/strlen.o \
			$(DIR)/strrchr.o \
			$(DIR)/tolower.o \
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
