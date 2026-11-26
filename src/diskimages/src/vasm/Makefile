# vasm for the disk images: the 68k assembler with Motorola syntax.
#
# This replaces vasm's own Makefile (Unix/gcc), which builds for the host.
# src/diskimages/stdenv.mk supplies the cross compiler, flags and library,
# and builds into obj/ like the other programs.

ifndef MEGA_MICROS_DIR
$(error ERROR: MEGA_MICROS_DIR not set)
endif

# Which disk image to install on
DISKS = romdisk tools

# These have to be set before stdenv.mk is included, as its rules use them.

# Which assembler to build. (Not CPU and SYNTAX, as vasm's makefiles call
# them: stdenv.mk already uses CPU for the 68030 we compile for.)
VASM_CPU    = m68k
VASM_SYNTAX = mot

# The romdisk program's name
#PROGRAM_BASENAME = vasm$(VASM_CPU)_$(VASM_SYNTAX)
PROGRAM_BASENAME = vasm

# Output formats, as in make.rules. vasm.c refers to every one of them, so
# they all have to be built in.
OUTFMTS = -DOUTAOF -DOUTAOUT -DOUTBIN -DOUTCOFF -DOUTELF -DOUTGST -DOUTHANS \
          -DOUTHUNK -DOUTIHEX -DOUTO65 -DOUTPAP -DOUTSREC -DOUTTOS -DOUTVOBJ \
          -DOUTWOZ -DOUTXFIL

CSOURCES = vasm.c atom.c expr.c symtab.c symbol.c error.c parse.c reloc.c \
           hugeint.c cond.c listing.c source.c supp.c dwarf.c osdep.c \
           cpus/$(VASM_CPU)/cpu.c syntax/$(VASM_SYNTAX)/syntax.c \
           output_test.c output_elf.c output_bin.c output_vobj.c \
           output_hunk.c output_aout.c output_tos.c output_xfile.c \
           output_srec.c output_cdef.c output_ihex.c output_o65.c \
           output_gst.c output_woz.c output_pap.c output_hans.c \
           output_coff.c output_aof.c
ASMSOURCES =
CINCLUDES =

-include $(MEGA_MICROS_DIR)/src/diskimages/stdenv.mk

# vasm is third-party C90 code: build it without -Werror/-pedantic/-Wextra.
# compat/ comes first, so its stddef.h wraps the library's (see there).
# Its style triggers thousands of harmless -Wmissing-braces/-Wparentheses
# warnings, so those are off; the rest stay on.
CFLAGS := -Icompat $(filter-out -std=c11 -Werror -pedantic -Wextra,$(CFLAGS)) -std=gnu99 \
          -Wno-missing-braces -Wno-parentheses -Wno-maybe-uninitialized
EXTRA_CFLAGS += -I. -Icpus/$(VASM_CPU) -Isyntax/$(VASM_SYNTAX) -DUNIX $(OUTFMTS)

# Objects for cpus/ and syntax/ go in matching folders under obj/
OBJ_SUBDIRS = $(OBJ_DIR)/cpus/$(VASM_CPU) $(OBJ_DIR)/syntax/$(VASM_SYNTAX)

$(OBJECTS): | $(OBJ_SUBDIRS)

$(OBJ_SUBDIRS):
	$(MKDIR) $@
