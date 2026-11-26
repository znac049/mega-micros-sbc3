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

# The disk images, and which programs go on them.
#
# Each disk is a directory tree, trees/<disk>, which is turned into an
# ext2 image. Every program says which disks it goes on, by setting DISKS
# in its Makefile before including stdenv.mk:
#
#   DISKS = romdisk tools
#
# A program that doesn't set DISKS goes on DEFAULT_DISKS.
#
# romdisk is special: micromon's Makefile builds its image, sized to fit
# in the ROM after the monitor. Every other disk listed here gets an image
# of its own, images/<disk>.img, of DISK_SIZE_<disk> MB.
#
# Fixed files for a disk (configuration, scripts, documentation...) go in
# files/<disk>/, and are copied into its tree before the programs are.

# Every disk. A program naming a disk that isn't here is an error.
ALL_DISKS := romdisk tools rescue

# Where programs go if they don't say
DEFAULT_DISKS := romdisk

# Image sizes, in MB, for the disks that get their own image
DISK_SIZE_tools  := 16
DISK_SIZE_rescue := 4

# The monitor's ext2 code only handles 2 KB blocks
DISK_BLOCK_SIZE := 2048

# Fixed files (from files/<disk>/) to strip of their symbols as they're
# copied into a disk's tree, keeping only those in STRIP_KEEP_<disk>.
# The NetBSD kernel keeps dt_blob_start, which the netbsd command looks up
# to find the device tree. (The linker may only store that name as the
# tail of _dt_blob_start, so keep both.)
STRIP_FILES_romdisk := boot/netbsd
STRIP_KEEP_romdisk  := dt_blob_start _dt_blob_start

DISK_STRIP := m68k-linux-gnu-strip

# The disks that micromon puts in the ROM, rather than in images/
ROM_DISKS := romdisk

IMAGE_DISKS := $(filter-out $(ROM_DISKS),$(ALL_DISKS))

DISKIMAGES_DIR := $(MEGA_MICROS_DIR)/src/diskimages
DISK_TREES_DIR := $(DISKIMAGES_DIR)/trees
DISK_IMAGES_DIR := $(DISKIMAGES_DIR)/images
DISK_FILES_DIR := $(DISKIMAGES_DIR)/files
