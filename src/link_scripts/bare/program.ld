OUTPUT_FORMAT("binary")
ENTRY(_start)

STARTUP(crt0.o)

INCLUDE "memory.ld"

PROVIDE(_STACK_SIZE     = 256k);          /* 128KB default stack size     */

REGION_ALIAS("TEXT_REGION", RAM)
REGION_ALIAS("DATA_REGION", RAM)

CODE_START = _STACK_SIZE;

INCLUDE "sections.ld"
