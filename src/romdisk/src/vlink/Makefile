# vlink for the romdisk: Frank Wille's portable multi-format linker.
#
# This replaces vlink's own Makefile (Unix/gcc), which builds for the host.
# The romdisk's stdenv.mk supplies the cross compiler, flags and library,
# and builds into obj/ like the other romdisk programs.
#
# vlink's sources are used unmodified (its licence asks for that).

ifndef MEGA_MICROS_DIR
$(error ERROR: MEGA_MICROS_DIR not set)
endif

# These have to be set before stdenv.mk is included, as its rules use them.

PROGRAM_BASENAME = vlink

# Every .c here is part of vlink (make.rules lists them all, plus
# version.c), so a newer release with more targets needs no change here.
CSOURCES   = $(sort $(wildcard *.c))
ASMSOURCES =
CINCLUDES  =

-include $(MEGA_MICROS_DIR)/src/romdisk/stdenv.mk

# vlink is third-party C99 code: build it without -Werror/-pedantic/-Wextra.
# Its style triggers many harmless -Wparentheses and similar warnings,
# so those are off; the rest stay on.
CFLAGS := $(filter-out -std=c11 -Werror -pedantic -Wextra,$(CFLAGS)) -std=gnu99 \
          -Wno-parentheses -Wno-missing-braces -Wno-maybe-uninitialized \
          -Wno-unused-but-set-variable -Wno-unused-variable

# With no -b option, write a 68k ELF executable. (config.h only picks a
# default for the systems it knows, so it leaves this one to us.)
EXTRA_CFLAGS += -I. -DDEFTARGET=\"elf32m68k\"
