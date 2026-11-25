/*
 * The library's <stddef.h>, without the control-character names it adds
 * (BELL, BS, CR, LF): vasm's 68k code uses BS and others as identifiers.
 */
#include_next <stddef.h>

#undef BELL
#undef BS
#undef CR
#undef LF
