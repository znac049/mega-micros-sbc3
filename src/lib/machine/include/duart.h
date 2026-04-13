#pragma once

#include <ctype.h>

/* Duart Register Addresses */
#define duart_base ((volatile unsigned char*) 0xad0001)

#define duart_mr1a          ((volatile unsigned char*) duart_base)
#define duart_mr2a          ((volatile unsigned char*) duart_base)
#define duart_sra           ((volatile unsigned char*) duart_base+2)
#define duart_csra          ((volatile unsigned char*) duart_base+2)
#define duart_cra           ((volatile unsigned char*) duart_base+4)
#define duart_rba           ((volatile unsigned char*) duart_base+6)
#define duart_tba           ((volatile unsigned char*) duart_base+6)
#define duart_ipcr          ((volatile unsigned char*) duart_base+8)
#define duart_acr           ((volatile unsigned char*) duart_base+8)
#define duart_isr           ((volatile unsigned char*) duart_base+10)
#define duart_imr           ((volatile unsigned char*) duart_base+10)
#define duart_cur           ((volatile unsigned char*) duart_base+12)
#define duart_ctur          ((volatile unsigned char*) duart_base+12)
#define duart_clr           ((volatile unsigned char*) duart_base+14)
#define duart_ctlr          ((volatile unsigned char*) duart_base+14)
#define duart_mr1b          ((volatile unsigned char*) duart_base+16)
#define duart_mr2b          ((volatile unsigned char*) duart_base+16)
#define duart_srb           ((volatile unsigned char*) duart_base+18)
#define duart_csrb          ((volatile unsigned char*) duart_base+18)
#define duart_crb           ((volatile unsigned char*) duart_base+20)
#define duart_rbb           ((volatile unsigned char*) duart_base+22)
#define duart_tbb           ((volatile unsigned char*) duart_base+22)
#define duart_ivr           ((volatile unsigned char*) duart_base+24)
#define duart_opcr          ((volatile unsigned char*) duart_base+26)
#define duart_start_counter ((volatile unsigned char*) duart_base+28)
#define duart_opr_set       ((volatile unsigned char*) duart_base+28)
#define duart_stop_counter  ((volatile unsigned char*) duart_base+30)
#define duart_opr_reset     ((volatile unsigned char*) duart_base+30)

/* Status Register A and B */
#define SR_RXRDY 0x01
#define SR_RX_READY 0x01
#define SR_TXRDY 0x04
#define SR_TX_READY 0x04
#define SR_TX_EMPTY 0x08

/* Command Register A and B */
#define CR_NOP          0x00
#define CR_RESET_MR_PTR 0x10
#define CR_RESET_RX     0x20
#define CR_RESET_TX     0x30
#define CR_RESET_ERROR  0x40
#define CR_RESET_BREAK  0x50
#define CR_START_BREAK  0x60
#define CR_STOP_BREAK   0x70
#define CR_SET_EXT_RX   0x80
#define CR_CLEAR_EXT_RX 0x90
#define CR_SET_EXT_TX   0xa0
#define CR_CLEAR_EXT_TX 0xb0
#define CR_STANDBY_MODE 0xc0
#define CR_ACTIVE_MODE  0xd0

#define CR_ENABLE_TX    0x04
#define CR_DISABLE_TX   0x08
#define CR_ENABLE_RX    0x01
#define CR_DISABLE_RX   0x02

/* Interrupt Status Register */
#define GLOBAL_ISR_TXRDY_A 0x01
#define GLOBAL_ISR_RXRDY_A 0x02
#define GLOBAL_ISR_TXRDY_B 0x10
#define GLOBAL_ISR_RXRDY_B 0x20

#define ISR_TX_READY    0x01
#define ISR_RX_READY    0x02

int _getchar(void);
int _buffered_getchar(void);
int _polled_getchar(void);
void _putchar(int ch);
void _buffered_putchar(int);
void _polled_putchar(int ch);
int _char_available(void);
int _polled_char_available(void);
int _polled_char_available(void);

void _claim_duart(void);
void _release_duart(void);
void clear_led(int);
void set_led(int);
