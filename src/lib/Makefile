ifndef MEGA_MICROS_DIR
$(error MEGA_MICROS_DIR is not set)
endif

CPU?=68030
EXTRA_CFLAGS?=-g -O2
EXTRA_CXXFLAGS?=-g -O2
DEFINES:=$(DEFINES)
FLAGS=-ffreestanding -ffunction-sections -fdata-sections				\
			-nostartfiles -Wall -Wpedantic -Werror      				\
			-mcpu=$(CPU) -march=$(CPU) -mtune=$(CPU) -msoft-float		\
			-nostdinc													\
			$(DEFINES)
CFLAGS=-std=c11 $(FLAGS)
CXXFLAGS=-std=c++20 -fno-exceptions -fno-rtti $(FLAGS)
ASFLAGS=
VASMFLAGS=-Felf -m$(CPU) -quiet $(DEFINES)
ARFLAGS=

GCC_68K_PFX=m68k-linux-gnu
CC=$(GCC_68K_PFX)-gcc
CXX=$(GCC_68K_PFX)-g++
AS=$(GCC_68K_PFX)-as
LD=$(GCC_68K_PFX)-ld
NM=$(GCC_68K_PFX)-nm
AR=$(GCC_68K_PFX)-ar
RANLIB=$(GCC_68K_PFX)-ranlib
VASM=vasmm68k_mot
RM_F=rm -f
RM_RF=$(RM_F) -r
MKDIR_P=mkdir -p
CP_R=cp -r

INSTALL_DIR=$(MEGA_MICROS_DIR)/built

ifeq ($(BAREMETAL),true)
INSTALL_LIB=$(INSTALL_DIR)/lib/bare
SCRIPT_DIR=../link_scripts/bare
INSTALL_INC=$(INSTALL_DIR)/include
OBJ_DIR=bare/
DEFINES += -DBAREMETAL
CRT0_OBJS=crt0.o rom_crt0.o
else
INSTALL_LIB=$(INSTALL_DIR)/lib/bios
SCRIPT_DIR=../link_scripts/bios
INSTALL_INC=$(INSTALL_DIR)/include
OBJ_DIR=bios/
DEFINES += -DUSEBIOS
CRT0_OBJS=crt0.o
endif

TARGET_LIB=libmega.a
USERLIB=$(OBJ_DIR)libmega.a

# Avoid plain "make" building only the first target encountered, causing confusion
decoy: all
	@echo Please use \"make all\" to build all the libraries.

VPATH = machine stdlib nonstdlib 

include machine/include.mk
include stdlib/include.mk
include crt0/include.mk
include nonstdlib/include.mk

$(info VPATH is $(VPATH))

ALL_OBJS += $(addprefix $(OBJ_DIR),$(OBJECTS))

$(OBJ_DIR)%.o : %.c
	$(CC) -c $(CFLAGS) $(EXTRA_CFLAGS) -o $@ $<

$(OBJ_DIR)%.o : %.cpp
	$(CXX) -c $(CXXFLAGS) $(EXTRA_CXXFLAGS) -o $@ $<

$(OBJ_DIR)%.o : %.S
	$(AS) $(ASFLAGS) -o $@ $<

$(OBJ_DIR)%.o : %.asm
	$(VASM) $(VASMFLAGS) -o $@ $<

.PHONY: all bare bios build install actual_install clean actual_clean decoy 

all: bare bios

build: $(USERLIB) $(ALL_OBJS) $(CRT0_OBJS)


bare: $(USERLIB)
	make build BAREMETAL=true

bios:
	make build BAREMETAL=false

$(USERLIB): $(ALL_OBJS)
	$(AR)	$(ARFLAGS) rs $@ $^
	$(RANLIB) $@

actual_install: $(USERLIB)
	$(MKDIR_P) $(INSTALL_INC)
	$(MKDIR_P) $(INSTALL_LIB)
	$(MKDIR_P) $(INSTALL_DIR)/ld
	$(CP_R) $(INCLUDES) $(INSTALL_INC)
	$(CP_R) $(USERLIB) $(INSTALL_LIB)
	$(CP_R) $(SCRIPT_DIR)/* $(INSTALL_LIB)

install:
	make actual_install BAREMETAL=true
	make actual_install BAREMETAL=false

clean:
	make actual_clean BAREMETAL=true
	make actual_clean BAREMETAL=false

actual_clean: 
	$(RM_F) $(ALL_OBJS) $(USERLIB)
