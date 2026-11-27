OUTPUT_FORMAT("binary")
ENTRY(_start)

/* STARTUP(mon_crt0.o) */

INCLUDE "memory.ld"

/* The monitor's stack goes straight after its variables. 16K is plenty:  */
/* its deepest calls take about 12K. Programs get stacks of their own     */
/* (see go_cmd.c), so this is only the monitor's.                         */
_MON_STACK_SIZE = 16k;

REGION_ALIAS("TEXT_REGION", ROM)
REGION_ALIAS("DATA_REGION", RAM)

CODE_START = ORIGIN(ROM);
DATA_START = 0x00000400;

INCLUDE "sections.ld"

/* Above the stack is the heap; programs load at _PROGRAM_BASE (memory.ld) */
_mon_stack_bottom = ALIGN(_bss_end, 16);
_INITIAL_STACK = _mon_stack_bottom + _MON_STACK_SIZE - 4;

ASSERT(_INITIAL_STACK + 4 <= _PROGRAM_BASE, "The monitor's variables and stack don't fit below _PROGRAM_BASE")
