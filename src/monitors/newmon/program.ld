OUTPUT_FORMAT("binary")
ENTRY(_start)

/* STARTUP(crt0.o) */

MEMORY
{
  RAM       : org = 0x0000A000,  l = 0x003F6000
  ROM       : org = 0x00C00000,  l = 0x00400000
}

PROVIDE(_LOAD_ADDRESS   = 0x00010000);  /* firmware KERNEL_LOAD_ADDRESS */
PROVIDE(_RUN_ADDRESS    = 0x00010000);  /* start of user memory         */

SECTIONS
{
  .pretext _RUN_ADDRESS : AT(_LOAD_ADDRESS)
  {
    _pretext = .;
    KEEP(*(.pretext))
    . = ALIGN(4);
    _pretext_end = .;
  } > RAM

  .text.init ALIGN(4) :
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
