LIB=machine

SUBDIR := $(shell dirname $(lastword $(MAKEFILE_LIST)))

LIBOBJECTS := $(LIBOBJECTS) \
			$(SUBDIR)/blocks.o \
			$(SUBDIR)/dir.o \
			$(SUBDIR)/dump.o \
			$(SUBDIR)/endian.o \
			$(SUBDIR)/ext2.o \
			$(SUBDIR)/file.o

LIBINCLUDES=$(SUBDIR)/include

# UPPERLIB := $(shell echo $(LIB) | tr '[:lower:]' '[:upper:]')
# BINARY := lib$(LIB).a
CFLAGS  := $(CFLAGS) -I$(LIBINCLUDES)
OBJECTS := $(OBJECTS) $(LIBOBJECTS)
# INCLUDES := $(INCLUDES) $(SUBDIR)/include/*
# LIBS := $(LIBS) $(DIR)/$(BINARY)

$(DIR)/$(BINARY): $(LIBOBJECTS)
	$(AR) $(ARFLAGS) rs $@ $^
	$(RANLIB) $@
