LIB=crt0
LIBOBJECTS=$(DIR)/crt0.o

DIR := $(shell dirname $(lastword $(MAKEFILE_LIST)))
OBJECTS := $(OBJECTS) $(LIBOBJECTS)

