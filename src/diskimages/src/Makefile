# MIT License

# Copyright (c) 2026 Bob Green

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

# Builds every program for the disk images. Each subdirectory of this
# directory that has its own Makefile is one program; its Makefile says
# which disks it goes on (see ../disks.mk).
#
#   make            build them all
#   make install    build them all and copy each onto its disks' trees
#   make clean      clean them all
#   make list       show the programs found, and the disks each goes on
#
#   make sayhi      build just one program (likewise install-sayhi, clean-sayhi)
#
# Builds can run in parallel: make -j4 install
#
# Normally this is run from ../Makefile, which also empties the trees
# first and makes the images.

ifndef MEGA_MICROS_DIR
$(error ERROR: MEGA_MICROS_DIR not set)
endif

# Every subdirectory with a Makefile is a program
# Programs not to build, if any (e.g. "make SKIP_PROGRAMS=vasm")
SKIP_PROGRAMS ?=

PROGRAMS := $(filter-out $(SKIP_PROGRAMS),$(sort $(patsubst %/Makefile,%,$(wildcard */Makefile))))

INSTALL_TARGETS := $(addprefix install-,$(PROGRAMS))
CLEAN_TARGETS   := $(addprefix clean-,$(PROGRAMS))

all: $(PROGRAMS)

install: $(INSTALL_TARGETS)

clean: $(CLEAN_TARGETS)

list:
	@for p in $(PROGRAMS); do $(MAKE) -s --no-print-directory -C $$p disks; done

$(PROGRAMS):
	$(MAKE) -C $@

# Static pattern rules, because make won't use ordinary pattern
# rules for phony targets
$(INSTALL_TARGETS): install-%:
	$(MAKE) -C $* install

$(CLEAN_TARGETS): clean-%:
	$(MAKE) -C $* clean

.PHONY: all install clean list $(PROGRAMS) $(INSTALL_TARGETS) $(CLEAN_TARGETS)
