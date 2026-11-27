MEMORY
{
  RAM (wx)  : org = 0x00000000,  len = 4M 
  ROM (rx)  : org = 0x00C00000,  len = 4M
}

/* Where programs run by the monitor are loaded. It's fixed, so programs */
/* don't need relinking when the monitor changes; the monitor's own      */
/* variables and stack must fit below it (micromon.ld checks)            */
_PROGRAM_BASE = 0x00040000;
