OUTPUT_FORMAT("binary")
ENTRY(_start)

MEMORY
{
  RAM       : org = 0x00200000,  l = 0x00200000
  ROM       : org = 0x00CO0000,  l = 0x00400000
}

/* program configuration symbols that may be useful to override */
/* using linker option: --defsym=<symbol_name>=<value>          */
PROVIDE(_STACK_SIZE     = 0x00004000);  /* 16KB default stack size      */

/* firmware absolute addresses (generally not useful to override)  */
PROVIDE(_INITIAL_STACK  = 0x00000000);  /* firmware stack top (mem top) */
PROVIDE(_WARM_BOOT      = 0x00000004);  /* firmware warm boot address   */

PROVIDE(_MFP_VECTORS    = 0x00000100);  /* MFP interrupt vector base    */

/* ROM absolute addresses */
PROVIDE(_FIRMWARE       = 0x00FC0000);  /* firmware address             */
PROVIDE(_FIRMWARE_REV   = 0x00FC0400);  /* firmware revision code       */

/* NOTE: rev1.1 used 0x00028000 (but init now position independent)     */
PROVIDE(_LOAD_ADDRESS   = 0x00200000);  /* firmware KERNEL_LOAD_ADDRESS */
PROVIDE(_RUN_ADDRESS    = 0x00280000);  /* start of user memory         */

SECTIONS
{
  .text.init _RUN_ADDRESS : AT(_LOAD_ADDRESS)
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
