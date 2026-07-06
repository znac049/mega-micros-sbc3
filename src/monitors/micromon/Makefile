ifndef MEGA_MICROS_DIR
$(error ERROR: MEGA_MICROS_DIR not set)
endif

.PHONY: first

first: rom
	$(OBJCOPY) -O binary $(ROM_ELF) $(PROGRAM_BASENAME)_rom.bin
	scp $(PROGRAM_BASENAME)_rom.bin $(REMOTE_USER):$(REMOTE_DIR)

-include $(MEGA_MICROS_DIR)/src/stdenv.mk
FLAGS:=$(FLAGS) -Wno-array-bounds