# Unix, using gcc

ifndef MEGA_MICROS_DIR
$(error ERROR: MEGA_MICROS_DIR not set)
endif

-include $(MEGA_MICROS_DIR)/src/romdisk/stdenv.mk

TARGET =
TARGETEXTENSION =

CPU = m68k
SYNTAX = mot

CCOUT = -o $(DUMMY)
CFLAGS = -c -std=c90 -g -pedantic -Wno-long-long -DUNIX $(OUTFMTS)

LD = $(CC)
LDOUT = $(CCOUT)
LDFLAGS = -lm

RM = rm -f

include make.rules
