#pragma once

/* PI/T Timer Register Addresses */
#define pit_base ((unsigned char*) 0xaf0001)

#define pit_pgcr  ((unsigned char*) pit_base)
#define pit_psrr  ((unsigned char*) pit_base+2)
#define pit_paddr ((unsigned char*) pit_base+4)
#define pit_pbddr ((unsigned char*) pit_base+6)
#define pit_pcddr ((unsigned char*) pit_base+8)
#define pit_pivr  ((unsigned char*) pit_base+10)
#define pit_pacr  ((unsigned char*) pit_base+12)
#define pit_pbcr  ((unsigned char*) pit_base+14)
#define pit_padr  ((unsigned char*) pit_base+16)
#define pit_pbdr  ((unsigned char*) pit_base+18)
#define pit_paar  ((unsigned char*) pit_base+20)
#define pit_pbar  ((unsigned char*) pit_base+22)
#define pit_pcdr  ((unsigned char*) pit_base+24)
#define pit_psr   ((unsigned char*) pit_base+26)

#define pit_tcr   ((unsigned char*) pit_base+32)
#define pit_tivr  ((unsigned char*) pit_base+34)
#define pit_cprh  ((unsigned char*) pit_base+38)
#define pit_cprm  ((unsigned char*) pit_base+40)
#define pit_cprl  ((unsigned char*) pit_base+42)
#define pit_cntrh ((unsigned char*) pit_base+46)
#define pit_cntrm ((unsigned char*) pit_base+48)
#define pit_cntrl ((unsigned char*) pit_base+50)
#define pit_tsr   ((unsigned char*) pit_base+52)

void _pit_reset(void);
unsigned int _pit_get_counter(void);
unsigned int _pit_set_counter(unsigned int);
