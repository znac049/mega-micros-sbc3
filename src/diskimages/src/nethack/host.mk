# The parts of the NetHack build that run on the PC: makedefs, dgn_comp,
# lev_comp and dlb, which make the generated headers and the data file.
#
# Some of what they write is binary (the compiled levels, the dungeon and
# quest data), laid out as the program that writes it lays out its own
# structures. So they're built for the board's CPU - as m68k Linux
# programs, with the cross compiler's own C library - and run under
# qemu-m68k: what they write is then just what the game expects to read.
#
# Included by the Makefile.

HOST_CC    := m68k-linux-gnu-gcc
HOST_DIR   := obj/host
GEN_INC    := obj/include
GEN_SRC    := obj/src
DAT_DIR    := obj/dat

HOST_CFLAGS := -O2 -static -w -DMEGA_MICROS -I$(GEN_INC) -Iinclude

# Run one of them, from the directory they expect (paths like ../dat/...)
RUN = cd $(HOST_DIR) && $(QEMU) ./$(1)

UTIL_COMMON := $(HOST_DIR)/monst.o $(HOST_DIR)/objects.o
MAKEDEFS_OBJS := $(HOST_DIR)/makedefs.o $(UTIL_COMMON)
LEVCOMP_OBJS := $(HOST_DIR)/lev_yacc.o $(HOST_DIR)/lev_lex.o $(HOST_DIR)/lev_main.o \
                $(HOST_DIR)/alloc.o $(HOST_DIR)/panic.o $(HOST_DIR)/drawing.o \
                $(HOST_DIR)/decl.o $(UTIL_COMMON)
DGNCOMP_OBJS := $(HOST_DIR)/dgn_yacc.o $(HOST_DIR)/dgn_lex.o $(HOST_DIR)/dgn_main.o \
                $(HOST_DIR)/alloc.o $(HOST_DIR)/panic.o
DLB_OBJS := $(HOST_DIR)/dlb_main.o $(HOST_DIR)/dlb.o $(HOST_DIR)/alloc.o $(HOST_DIR)/panic.o

GEN_HEADERS := $(GEN_INC)/onames.h $(GEN_INC)/pm.h $(GEN_INC)/vis_tab.h $(GEN_INC)/date.h
GEN_SOURCES := $(GEN_SRC)/vis_tab.c

# Everything that goes into nhdat (as the Unix Makefile.top has it)
DAT_HELP := help hh cmdhelp keyhelp history opthelp wizhelp
DAT_VAR  := bogusmon data engrave epitaph oracles options quest.dat rumors
DAT_TEXT := $(DAT_HELP) tribute license symbols \
            data.base rumors.tru rumors.fal oracles.txt quest.txt \
            engrave.txt epitaph.txt bogusmon.txt dungeon.def

LEV_SOURCES := $(wildcard dat/*.des)

$(HOST_DIR) $(GEN_INC) $(GEN_SRC) $(DAT_DIR):
	$(MKDIR) $@

$(HOST_DIR)/%.o: src/%.c | $(HOST_DIR) $(GEN_INC)
	$(HOST_CC) -c $(HOST_CFLAGS) -o $@ $<

$(HOST_DIR)/%.o: util/%.c | $(HOST_DIR) $(GEN_INC)
	$(HOST_CC) -c $(HOST_CFLAGS) -o $@ $<

$(HOST_DIR)/%.o: sys/share/%.c | $(HOST_DIR) $(GEN_INC)
	$(HOST_CC) -c $(HOST_CFLAGS) -o $@ $<

# makedefs comes first: the others need the headers it makes
$(HOST_DIR)/makedefs: $(MAKEDEFS_OBJS)
	$(HOST_CC) -static -o $@ $^

$(HOST_DIR)/lev_comp: $(LEVCOMP_OBJS)
	$(HOST_CC) -static -o $@ $^

$(HOST_DIR)/dgn_comp: $(DGNCOMP_OBJS)
	$(HOST_CC) -static -o $@ $^

$(HOST_DIR)/dlb: $(DLB_OBJS)
	$(HOST_CC) -static -o $@ $^

# (makedefs' own objects can't wait for the headers it makes)
$(filter-out $(MAKEDEFS_OBJS),$(LEVCOMP_OBJS) $(DGNCOMP_OBJS) $(DLB_OBJS)): $(GEN_INC)/onames.h $(GEN_INC)/pm.h

# makedefs writes to ../include, ../src and ../dat, relative to where it runs
$(GEN_INC)/onames.h: $(HOST_DIR)/makedefs | $(DAT_DIR)
	$(call RUN,makedefs) -o

$(GEN_INC)/pm.h: $(HOST_DIR)/makedefs
	$(call RUN,makedefs) -p

$(GEN_INC)/vis_tab.h $(GEN_SRC)/vis_tab.c: $(HOST_DIR)/makedefs | $(GEN_SRC)
	$(call RUN,makedefs) -z

$(GEN_INC)/date.h: $(HOST_DIR)/makedefs | $(DAT_DIR)
	$(call RUN,makedefs) -v

# The data, from the text files and level descriptions
$(DAT_DIR)/.copied: $(addprefix dat/,$(DAT_TEXT)) $(LEV_SOURCES) | $(DAT_DIR)
	cp $^ $(DAT_DIR)/
	touch $@

$(DAT_DIR)/nhdat: $(DAT_DIR)/.copied $(HOST_DIR)/makedefs $(HOST_DIR)/lev_comp \
                  $(HOST_DIR)/dgn_comp $(HOST_DIR)/dlb $(GEN_INC)/date.h
	cd $(DAT_DIR) && for opt in -d -r -h -q -s -e; do $(QEMU) ../host/makedefs $$opt || exit 1; done
	cd $(DAT_DIR) && $(QEMU) ../host/dgn_comp dungeon.pdf
	cd $(DAT_DIR) && for f in *.des; do $(QEMU) ../host/lev_comp $$f || exit 1; done
	cd $(DAT_DIR) && LC_ALL=C $(QEMU) ../host/dlb cf nhdat $(DAT_HELP) dungeon tribute \
		$$(ls *.lev) $(DAT_VAR)
