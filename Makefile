# Top-level Makefile: builds everything in the right order.
#
# The library, the monitor and the disk image programs must agree on
# things like the system call numbers in bios.h, so they're always built
# in this order:
#
#   1. lib        - the library, installed (with its headers) into built/
#   2. programs   - the programs in src/diskimages, installed onto the
#                   trees of the disks they go on
#   3. images     - an ext2 image of each disk but romdisk
#   4. monitor    - micromon, plus the romdisk image made from its tree
#
#   make            build all of them, in order
#   make rebuild    clean, then build everything from scratch
#   make clean      clean them all
#   make scp        build, then make the ROM image and copy it (micromon's scp)
#
# src/diskimages/disks.mk lists the disks and their sizes.
#
# Header changes are tracked across directories: changing a library header
# (bios.h, say) rebuilds the library objects, monitor objects and programs
# that include it, and anything linked with the library is relinked when it
# changes. "make rebuild" is there if you ever want a clean start.

MEGA_MICROS_DIR ?= $(CURDIR)
export MEGA_MICROS_DIR

LIB_DIR      = src/lib
DISKS_DIR    = src/diskimages
MONITOR_DIR  = src/micromon

.PHONY: all lib programs images monitor clean rebuild scp

all: monitor images

lib:
	$(MAKE) -C $(LIB_DIR) install

programs: lib
	$(MAKE) -C $(DISKS_DIR) trees

# Only the images: the trees were filled by programs
images: programs
	$(MAKE) -C $(DISKS_DIR) images-only

monitor: lib programs
	$(MAKE) -C $(MONITOR_DIR) romdisk

scp: lib programs
	$(MAKE) -C $(MONITOR_DIR) scp

clean:
	$(MAKE) -C $(MONITOR_DIR) clean
	$(MAKE) -C $(DISKS_DIR) clean
	$(MAKE) -C $(LIB_DIR) clean

# Run as separate makes, so the clean is finished before anything is built
rebuild:
	$(MAKE) clean
	$(MAKE) all
