ifeq ($(BAREMETAL),true)
CRT0_OBJECTS=\
			crt0.o \
			rom_crt0.o
else
CRT0_OBJECTS=\
			crt0.o
endif
