ifndef MEGA_MICROS_DIR
$(error MEGA_MICROS_DIR is not set)
endif

CPU?=68030
EXTRA_CFLAGS?=-g -O2
EXTRA_CXXFLAGS?=-g -O2
DEFINES:=$(DEFINES) -DROSCO_M68K_SYSLIBS
FLAGS=-ffreestanding -ffunction-sections -fdata-sections				\
			-nostartfiles -Wall -Wpedantic -Werror      				\
			-mcpu=$(CPU) -march=$(CPU) -mtune=$(CPU)    				\
			$(DEFINES)
CFLAGS=-std=c11 $(FLAGS)
CXXFLAGS=-std=c++20 -fno-exceptions -fno-rtti $(FLAGS)
ASFLAGS=-Felf -m$(CPU) -quiet $(DEFINES)
ARFLAGS=

GCC_68K_PFX=m68k-linux-gnu
CC=$(GCC_68K_PFX)-gcc
CXX=$(GCC_68K_PFX)-g++
AS=$(GCC_68K_PFX)-as
LD=$(GCC_68K_PFX)-ld
NM=$(GCC_68K_PFX)-nm
AR=$(GCC_68K_PFX)-ar
RANLIB=$(GCC_68K_PFX)-ranlib
AS=vasmm68k_mot
RM_F=rm -f
RM_RF=$(RM_F) -r
MKDIR_P=mkdir -p
CP_R=cp -r

INSTALL_DIR=$(MEGA_MICROS_DIR)/built
INSTALL_LIB=$(INSTALL_DIR)/lib
INSTALL_INC=$(INSTALL_DIR)/include

UBERLIB=libmega.a

# Avoid plain "make" building only the first target encountered, causing confusion
decoy:
	@echo Please use \"make all\" to build all the libraries.

include stdlib/include.mk
include machine/include.mk

# Keep this at the end, or it'll error out _sometimes_ and we don't really know why...
#include printf/include.mk

%.o : %.c
	$(CC) -c $(CFLAGS) $(EXTRA_CFLAGS) -o $@ $<

%.o : %.cpp
	$(CXX) -c $(CXXFLAGS) $(EXTRA_CXXFLAGS) -o $@ $<

%.o : %.S
	$(AS) $(ASFLAGS) -o $@ $<

%.o : %.asm
	$(AS) $(ASFLAGS) -o $@ $<

.PHONY: all install clean clobber

all: $(UBERLIB) $(LIBS)

$(UBERLIB): $(filter-out **/printf-*.o, $(OBJECTS))
	$(AR)	$(ARFLAGS) rs $@ $^
	$(RANLIB) $@

install: $(UBERLIB) $(LIBS)
	$(MKDIR_P) $(INSTALL_INC)
	$(MKDIR_P) $(INSTALL_LIB)
	$(MKDIR_P) $(INSTALL_DIR)/ld
	$(CP_R) $(INCLUDES) $(INSTALL_INC)
	$(CP_R) $(LIBS) $(INSTALL_LIB)
	$(CP_R) $(UBERLIB) $(INSTALL_LIB)
	$(CP_R) ../link_scripts/* $(INSTALL_DIR)/ld

clean: 
	$(RM_F) $(OBJECTS) $(LIBS) $(UBERLIB)

clobber: clean
	$(RM_RF)	$(INSTALL_DIR)

