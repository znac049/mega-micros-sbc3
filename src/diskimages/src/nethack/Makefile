# NetHack 3.6.7 for the disk images.
#
# The game is built for the board with the library, like the other
# programs. Its data - the compiled levels, dungeon, quest text, help
# files and so on - is made on the PC by NetHack's own utilities (see
# host.mk, which needs qemu-m68k) and packed into nhdat.
#
# On the disk, the program goes in bin/ and its data in games/nethack/,
# where its saved games, score list and lock files go too. The game finds
# that directory from where it was run: <disk>/bin/nethack uses
# <disk>/games/nethack (or give it -d <dir>).
#
# NetHack's own files are as released, except for include/unixconf.h,
# which hands over to include/megaconf.h for this board. The port itself
# is in sys/mega/.

ifndef MEGA_MICROS_DIR
$(error ERROR: MEGA_MICROS_DIR not set)
endif

DISKS = tools

PROGRAM_BASENAME = nethack

# The data needs qemu-m68k (from Debian's qemu-user package). Without it,
# NetHack is left out, rather than stopping the whole build.
QEMU ?= qemu-m68k

ifeq ($(shell command -v $(QEMU) 2>/dev/null),)

all install:
	@echo "*** NetHack not built: $(QEMU) not found (apt install qemu-user) ***"

clean:
	rm -rf obj

disks:
	@echo "$(PROGRAM_BASENAME): $(DISKS) (not built: no $(QEMU))"

.PHONY: all install clean disks

else

GAME_SOURCES := $(sort $(wildcard src/*.c))
TTY_SOURCES  := win/tty/getline.c win/tty/termcap.c win/tty/topl.c win/tty/wintty.c
SYS_SOURCES  := sys/unix/unixmain.c sys/unix/unixunix.c sys/share/pmatchregex.c \
                $(sort $(wildcard sys/mega/*.c))

CSOURCES   = $(GAME_SOURCES) $(TTY_SOURCES) $(SYS_SOURCES) obj/src/vis_tab.c
ASMSOURCES =
CINCLUDES  =

-include $(MEGA_MICROS_DIR)/src/diskimages/stdenv.mk

include host.mk

# NetHack is old-style C: build it without -Werror/-pedantic/-Wextra and
# with the warnings it's known for turned off
CFLAGS := $(filter-out -std=c11 -Werror -pedantic -Wextra,$(CFLAGS)) -std=gnu99 \
          -Wno-parentheses -Wno-missing-braces -Wno-unused-but-set-variable \
          -Wno-unused-variable -Wno-format-overflow -Wno-format-truncation
EXTRA_CFLAGS += -DMEGA_MICROS -Isys/mega/include -I$(GEN_INC) -Iinclude

# unixmain.c's main() is called by sys/mega/megamain.c, which sets up the
# environment first
$(OBJ_DIR)/sys/unix/unixmain.o: EXTRA_CFLAGS += -Dmain=unix_main

# The generated headers come first
OBJ_SUBDIRS := $(sort $(dir $(OBJECTS)))
$(OBJECTS): $(GEN_HEADERS) | $(OBJ_SUBDIRS)

$(filter-out $(GEN_INC) $(GEN_SRC),$(OBJ_SUBDIRS)):
	$(MKDIR) $@

# The game and nhdat
all: $(DAT_DIR)/nhdat

# The data, in games/nethack on each of the program's disks. sysconf and
# symbols are read as plain files; the rest is in nhdat.
GAME_DATA := $(DAT_DIR)/nhdat dat/license dat/symbols sys/mega/sysconf

install: install-data

install-data: $(DAT_DIR)/nhdat
	@for disk in $(DISKS); do \
		dest=$(DISK_TREES_DIR)/$$disk/games/nethack; \
		echo "  nethack data -> $$disk/games/nethack"; \
		$(MKDIR) $$dest/save && \
		cp $(GAME_DATA) $$dest/ && \
		touch $$dest/record $$dest/logfile $$dest/xlogfile $$dest/perm || exit 1; \
	done

.PHONY: install-data

endif
