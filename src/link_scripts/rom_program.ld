OUTPUT_FORMAT("binary")
ENTRY(_start)

STARTUP(rom_crt0.o)

INCLUDE "memory.ld"

PROVIDE(_STACK_SIZE     = 256k);          /* 128KB (less the size of the data section and vectors) default stack size     */

REGION_ALIAS("TEXT_REGION", ROM)
REGION_ALIAS("DATA_REGION", RAM)

CODE_START = ORIGIN(ROM);
DATA_START = 0x00000400;                  /* _STACK_SIZE; */

INCLUDE "sections.ld"
