#pragma once

/* Duart Register Addresses */
#define duart_base ((unsigned char*) 0xad0001)

#define duart_mr1           ((unsigned char*) duart_base)
#define duart_mr1a          ((unsigned char*) duart_base)
#define duart_mr1b          ((unsigned char*) duart_base)
#define duart_sra           ((unsigned char*) duart_base+2)
#define duart_csra          ((unsigned char*) duart_base+2)
#define duart_cra           ((unsigned char*) duart_base+4)
#define duart_rba           ((unsigned char*) duart_base+6)
#define duart_tba           ((unsigned char*) duart_base+6)
#define duart_ipcr          ((unsigned char*) duart_base+8)
#define duart_acr           ((unsigned char*) duart_base+8)
#define duart_isr           ((unsigned char*) duart_base+10)
#define duart_imr           ((unsigned char*) duart_base+10)
#define duart_cur           ((unsigned char*) duart_base+12)
#define duart_ctur          ((unsigned char*) duart_base+12)
#define duart_clr           ((unsigned char*) duart_base+14)
#define duart_ctlr          ((unsigned char*) duart_base+14)
#define duart_mr2           ((unsigned char*) duart_base+16)
#define duart_mr2a          ((unsigned char*) duart_base+16)
#define duart_mr2b          ((unsigned char*) duart_base+16)
#define duart_srb           ((unsigned char*) duart_base+18)
#define duart_csrb          ((unsigned char*) duart_base+18)
#define duart_crb           ((unsigned char*) duart_base+20)
#define duart_rbb           ((unsigned char*) duart_base+22)
#define duart_tbb           ((unsigned char*) duart_base+22)
#define duart_ivr           ((unsigned char*) duart_base+24)
#define duart_opcr          ((unsigned char*) duart_base+26)
#define duart_start_counter ((unsigned char*) duart_base+28)
#define duart_opr_set       ((unsigned char*) duart_base+28)
#define duart_stop_counter  ((unsigned char*) duart_base+30)
#define duart_opr_reset     ((unsigned char*) duart_base+30)

#define DUART_RXRDY 0
#define DUART_TXRDY 2

char _getchar(void);
void _putchar(char c);
int _char_available(void);

void set_led(int);
void clear_led(int);