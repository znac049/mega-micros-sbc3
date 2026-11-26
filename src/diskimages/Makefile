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

# Builds the disk images: the programs in src/, installed onto the trees
# of the disks they go on, and an ext2 image for each disk. disks.mk
# lists the disks.
#
#   make            build everything, fill the trees and make the images
#   make trees      build the programs and fill the trees, but make no images
#   make images     the same as make
#   make list       show each program and the disks it goes on
#   make clean      clean the programs, and remove the trees and images
#
#   make images-only    make the images from the trees as they are
#   make image-tools    make just one image (likewise)
#
# The trees are emptied and refilled each time, so a program taken off a
# disk (or removed) doesn't linger on it. romdisk's tree is filled here,
# but its image is made by micromon's Makefile, as it goes in the ROM.

ifndef MEGA_MICROS_DIR
$(error ERROR: MEGA_MICROS_DIR not set)
endif

include disks.mk

PROGRAMS_DIR := src

IMAGES := $(addprefix $(DISK_IMAGES_DIR)/,$(addsuffix .img,$(IMAGE_DISKS)))
IMAGE_TARGETS := $(addprefix image-,$(IMAGE_DISKS))

all: images

images: trees
	$(MAKE) images-only

images-only: $(IMAGE_TARGETS)

# Start each tree afresh: its fixed files, if any, then the programs
trees:
	rm -rf $(DISK_TREES_DIR)
	@for disk in $(ALL_DISKS); do \
		mkdir -p $(DISK_TREES_DIR)/$$disk || exit 1; \
		if [ -d $(DISK_FILES_DIR)/$$disk ]; then \
			cp -rp $(DISK_FILES_DIR)/$$disk/. $(DISK_TREES_DIR)/$$disk/ || exit 1; \
		fi; \
	done
	$(MAKE) -C $(PROGRAMS_DIR) install

# mkfs.ext2 -d copies a tree into a new filesystem, of exactly
# DISK_SIZE_<disk> MB.
$(IMAGE_TARGETS): image-%:
	@if [ -z "$(DISK_SIZE_$*)" ]; then \
		echo "No DISK_SIZE_$* in disks.mk"; exit 1; \
	fi
	mkdir -p $(DISK_IMAGES_DIR)
	rm -f $(DISK_IMAGES_DIR)/$*.img
	mkfs.ext2 -q -b $(DISK_BLOCK_SIZE) -m 1 -L $* -M //$* -t ext2 \
		-d $(DISK_TREES_DIR)/$* $(DISK_IMAGES_DIR)/$*.img $(DISK_SIZE_$*)M
	@echo "  $*.img: $(DISK_SIZE_$*) MB, $$(du -sk $(DISK_TREES_DIR)/$* | cut -f1) KB used"

list:
	@$(MAKE) -s --no-print-directory -C $(PROGRAMS_DIR) list

clean:
	$(MAKE) -C $(PROGRAMS_DIR) clean
	rm -rf $(DISK_TREES_DIR) $(DISK_IMAGES_DIR)

.PHONY: all images images-only trees list clean $(IMAGE_TARGETS)
