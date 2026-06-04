OUTPUT_FORMAT("binary")
ENTRY(_start)

STARTUP(crt0.o)

MEMORY
{
  RAM (wx)  : org = 0x00000000,  len = 4M
  ROM (rx)  : org = 0x00C00000,  len = 4M
}

/* program configuration symbols that may be useful to override */
/* using linker option: --defsym=<symbol_name>=<value>          */

PROVIDE(_STACK_SIZE     = 256k);  /* 128KB default stack size      */

PROVIDE(_INITIAL_STACK  = _STACK_SIZE-4);

PROVIDE(_RUN_ADDRESS    = _STACK_SIZE);  /* start of user memory         */

SECTIONS
{
  .text.init _RUN_ADDRESS : AT(_RUN_ADDRESS)
  {
    _init = .;
    KEEP(*(.init))      /* KEEP() "anchors" section for gc-sections */
    _init_end = .;
  } >RAM

  .text.postinit ALIGN(4) :
  {
    _postinit = .;
    KEEP(*(.postinit))  /* KEEP() "anchors" section for gc-sections */
    . = ALIGN(4);       /* long align for init.S copying */
    _postinit_end = .;
  } >RAM

  .text ALIGN(4) :
  {
    _code = .;
    *(.text*)
    *(.rodata*)
    _code_end = .;
  } >RAM

  . = DATA_SEGMENT_ALIGN(CONSTANT(MAXPAGESIZE), CONSTANT(COMMONPAGESIZE));

  .ctors ALIGN(4) :
  {
      _ctors = .;
      KEEP(*(.ctors))           /* Run in reverse order, so non-priority ones go first  */
      KEEP(*(SORT(.ctors.*)))   /* followed by ctors with priority */
      _ctors_end = .;
  } >RAM

  .dtors ALIGN(4) : 
  {
      _dtors = .;
      KEEP(*(SORT(.dtors.*)))   /* dtors with priority go first */
      KEEP(*(.dtors))           /* Followed by non-priority ones */
      _dtors_end = .;
  } >RAM

  .data ALIGN(4) :
  {
    _data_start = .;
    *(.data*)
    . = ALIGN(4);       /* long align for init.S copying */
    _data_end = .;
  } >RAM

  .bss ALIGN(4) :
  {
    _bss_start = .;
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);       /* long align for kinit clearing */
    _bss_end = .;
  } >RAM

  . = DATA_SEGMENT_END(.);

  _end = .;
}
