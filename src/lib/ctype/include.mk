LIBOBJECTS=isalnum.o \
			isalpha.o \
			isblank.o \
			isdigit.o \
			isspace.o \
			isxdigit.o \
			tolower.o \
			toupper.o

LIBINCLUDES=$(DIR)/include

DIR := $(shell dirname $(lastword $(MAKEFILE_LIST)))
CFLAGS  := $(CFLAGS) -I$(LIBINCLUDES)
OBJECTS := $(OBJECTS) $(LIBOBJECTS)
#INCLUDES := $(INCLUDES) $(DIR)/include/*