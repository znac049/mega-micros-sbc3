/* program configuration symbols that may be useful to override */
/* using linker option: --defsym=<symbol_name>=<value>          */

PROVIDE(_INITIAL_STACK  = _STACK_SIZE-4);

SECTIONS
{
  .pretext CODE_START :
  {
    _pretext_start = .;
    KEEP(*(.pretext))
    _pretext_end = .;
  } > TEXT_REGION

  .text.init ALIGN(4) :
  {
    _init_start = .;
    KEEP(*(.init))              /* KEEP() "anchors" section for gc-sections */
    _init_end = .;
  } > TEXT_REGION

  .text.postinit ALIGN(4) :
  {
    _postinit_start = .;
    KEEP(*(.postinit))          /* KEEP() "anchors" section for gc-sections */
    . = ALIGN(4);               /* long align for init.S copying */
    _postinit_end = .;
  } > TEXT_REGION

  .text ALIGN(4) :
  {
    _code_start = .;
    *(.text*)
    _code_end = .;
  } > TEXT_REGION

  .text ALIGN(4) :
  {
    _rodata_start = .;
    *(.rodata*)
    _rodata_end = .;
  } > TEXT_REGION

  .ctors ALIGN(4) :             /* Constructors */
  {
    _ctors_start = .;
    KEEP(*(.ctors))           /* Run in reverse order, so non-priority ones go first  */
    KEEP(*(SORT(.ctors.*)))   /* followed by ctors with priority */
    _ctors_end = .;
  } > TEXT_REGION

  .dtors ALIGN(4) :             /* Destructors */
  {
    _dtors_start = .;
    KEEP(*(SORT(.dtors.*)))   /* dtors with priority go first */
    KEEP(*(.dtors))           /* Followed by non-priority ones */
    _dtors_end = .;
  } > TEXT_REGION

  d_start = DEFINED(DATA_START) ? DATA_START : _dtors_end ;
  .data d_start : AT(_dtors_end)
  {
    _data_start = .;
    *(.data*)
    . = ALIGN(4);               /* long align for init.S copying */
    _data_end = .;
  } > DATA_REGION
  _data_load_start = LOADADDR(.data);

  .bss ALIGN(4) :
  {
    _bss_start = .;
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);               /* long align for kinit clearing */
    _bss_end = .;
  } > DATA_REGION

  _end = .;
}
