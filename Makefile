# Top-level Makefile: builds everything in the right order.
#
# The library, the monitor and the romdisk programs must agree on things
# like the system call numbers in bios.h, so they're always built in this
# order:
#
#   1. lib        - the library, installed (with its headers) into built/
#   2. programs   - the romdisk programs, installed into the romdisk tree
#   3. monitor    - micromon, plus the romdisk image made from that tree
#
#   make            build all three, in order
#   make rebuild    clean, then build everything from scratch
#   make clean      clean all three
#   make scp        build, then make the ROM image and copy it (micromon's scp)
#
# Header changes are tracked across directories: changing a library header
# (bios.h, say) rebuilds the library objects, monitor objects and programs
# that include it, and anything linked with the library is relinked when it
# changes. "make rebuild" is there if you ever want a clean start.

MEGA_MICROS_DIR ?= $(CURDIR)
export MEGA_MICROS_DIR

LIB_DIR      = src/lib
PROGRAMS_DIR = src/romdisk/src
MONITOR_DIR  = src/micromon

.PHONY: all lib programs monitor clean rebuild scp

all: monitor

lib:
	$(MAKE) -C $(LIB_DIR) install

programs: lib
	$(MAKE) -C $(PROGRAMS_DIR) install

monitor: lib programs
	$(MAKE) -C $(MONITOR_DIR) romdisk

scp: lib programs
	$(MAKE) -C $(MONITOR_DIR) scp

clean:
	$(MAKE) -C $(MONITOR_DIR) clean
	$(MAKE) -C $(PROGRAMS_DIR) clean
	$(MAKE) -C $(LIB_DIR) clean

# Run as separate makes, so the clean is finished before anything is built
rebuild:
	$(MAKE) clean
	$(MAKE) all
