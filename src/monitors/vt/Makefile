ifndef MEGA_MICROS_DIR
$(error ERROR: MEGA_MICROS_DIR not set)
endif

-include $(MEGA_MICROS_DIR)/src/stdenv.mk
FLAGS:=$(FLAGS) -Wno-array-bounds -Wno-unused-parameter -nostdinc
LDSCRIPT:=program.ld

zob:
	echo $(SREC)