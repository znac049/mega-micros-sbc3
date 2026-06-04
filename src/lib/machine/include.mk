LIB=machine
LIBOBJECTS=\
			$(DIR)/_cf.o \
			$(DIR)/_duart.o \
			$(DIR)/cf.o \
			$(DIR)/duart2.o \
			$(DIR)/leds.o \
			$(DIR)/_misc.o \
			$(DIR)/misc.o \
			$(DIR)/pit.o \
			$(DIR)/pre_main.o \
			$(DIR)/safeio.o \
			$(DIR)/termio.o \
			$(DIR)/_vectors.o \
			$(DIR)/vectors.o \

LIBINCLUDES=$(DIR)/include

DIR := $(shell dirname $(lastword $(MAKEFILE_LIST)))
UPPERLIB := $(shell echo $(LIB) | tr '[:lower:]' '[:upper:]')
BINARY := lib$(LIB).a
CFLAGS  := $(CFLAGS) -I$(LIBINCLUDES)
OBJECTS := $(OBJECTS) $(LIBOBJECTS)
INCLUDES := $(INCLUDES) $(DIR)/include/*
LIBS := $(LIBS) $(DIR)/$(BINARY)

$(DIR)/$(BINARY): $(LIBOBJECTS)
	$(AR) $(ARFLAGS) rs $@ $^
	$(RANLIB) $@
