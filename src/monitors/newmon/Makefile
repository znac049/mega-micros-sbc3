ifndef MEGA_MICROS_DIR
$(error ERROR: MEGA_MICROS_DIR not set)
endif

.PHONY: first

first: rom

-include $(MEGA_MICROS_DIR)/src/stdenv.mk
FLAGS:=$(FLAGS) -Wno-array-bounds