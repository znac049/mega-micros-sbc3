ifndef MEGA_MICROS_DIR
$(error ERROR: MEGA_MICROS_DIR not set)
endif

.PHONY: first inc_build romdisk scp

BAREMETAL=true

ROMDISK=romdisk.bin
ROMIMAGE=$(PROGRAM_BASENAME)_rom.bin
MONITORIMAGE=$(PROGRAM_BASENAME).bin

first: inc_build rom
	$(OBJCOPY) -O binary $(ROM_ELF) $(MONITORIMAGE)

scp: first romdisk
	cat $(MONITORIMAGE) inbetween.bin $(ROMDISK) >$(ROMIMAGE)
	scp $(ROMIMAGE) $(REMOTE_USER):$(REMOTE_DIR)

romdisk: first
	python3 makeromfs.py -i $(ROMDISK) -m $(MONITORIMAGE) -s 1

inc_build:
	./increment_build.sh

-include $(MEGA_MICROS_DIR)/src/stdenv.mk

FLAGS:=$(FLAGS) -Wno-array-bounds
TO_CLEAN += $(ROMDISK) $(ROMIMAGE) $(MONITORIMAGE)
