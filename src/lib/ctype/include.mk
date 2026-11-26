LIBOBJECTS=isalnum.o \
			isalpha.o \
			iscntrl.o \
			islower.o \
			ispunct.o \
			isupper.o \
			isblank.o \
			isdigit.o \
			isgraph.o \
			isprint.o \
			isspace.o \
			isxdigit.o \
			tolower.o \
			toupper.o

LIBINCLUDES=$(DIR)/include

DIR := $(shell dirname $(lastword $(MAKEFILE_LIST)))
CFLAGS  := $(CFLAGS) -I$(LIBINCLUDES)
OBJECTS := $(OBJECTS) $(LIBOBJECTS)
#INCLUDES := $(INCLUDES) $(DIR)/include/*