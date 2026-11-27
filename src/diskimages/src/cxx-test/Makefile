ifndef MEGA_MICROS_DIR
$(error ERROR: MEGA_MICROS_DIR not set)
endif

# A test of the C++ support: constructors and destructors of static
# objects, new and delete, virtual functions, templates, placement new
# and function-local statics

DISKS = tools

# It needs the C++ cross compiler (Debian/Ubuntu: g++-m68k-linux-gnu).
# Without it, the test is left out, rather than stopping the whole build.
ifeq ($(shell command -v m68k-linux-gnu-g++ 2>/dev/null),)

all install:
	@echo "*** cxx-test not built: m68k-linux-gnu-g++ not found (apt install g++-m68k-linux-gnu) ***"

clean:
	rm -rf obj

disks:
	@echo "cxx-test: $(DISKS) (not built: no m68k-linux-gnu-g++)"

.PHONY: all install clean disks

else

-include $(MEGA_MICROS_DIR)/src/diskimages/stdenv.mk

endif
