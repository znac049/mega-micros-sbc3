#pragma once

#include <ctype.h>

/* Duart Register Addresses */
#define duart_base ((unsigned char*) 0xad0001)

#define duart_mr1a          ((unsigned char*) duart_base)
#define duart_mr2a          ((unsigned char*) duart_base)
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
#define duart_mr1b          ((unsigned char*) duart_base+16)
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

/* Status Register A and B */
#define SR_RXRDY_BIT 0
#define SR_TXRDY_BIT 2

#define SR_RXRDY_MASK (1<<SR_RXRDY_BIT)
#define SR_TXRDY_MASK (1<<SR_TXRDY_BIT)

/* Command Register A and B */
#define CR_NOP          0x00
#define CR_RESET_MR_PTR 0x10
#define CR_RESET_RX     0x20
#define CR_RESET_TX     0x30
#define CR_RESET_ERROR  0x40

#define CR_ENABLE_TX    0x04
#define CR_DISABLE_TX   0x08

#define CR_ENABLE_RX    0x01
#define CR_DISABLE_RX   0x02

/* Interrupt Status Register */
#define GLOBAL_ISR_TXRDY_A_BIT 0
#define GLOBAL_ISR_RXRDY_A_BIT 1
#define GLOBAL_ISR_TXRDY_B_BIT 4
#define GLOBAL_ISR_RXRDY_B_BIT 5

#define GLOBAL_ISR_TXRDY_A_MASK (1<<GLOBAL_ISR_TXRDY_A_BIT)
#define GLOBAL_ISR_RXRDY_A_MASK (1<<GLOBAL_ISR_RXRDY_A_BIT)
#define GLOBAL_ISR_TXRDY_B_MASK (1<<GLOBAL_ISR_TXRDY_B_BIT)
#define GLOBAL_ISR_RXRDY_B_MASK (1<<GLOBAL_ISR_RXRDY_B_BIT)

#define CIRCULAR_BUFFER_SIZE 128

struct circular_buffer {
    uint8_t buffer[CIRCULAR_BUFFER_SIZE];
    volatile uint16_t insert;
    volatile uint16_t remove;
};

int _getchar(void);
void _putchar(int ch);
int _char_available(void);

void _claim_duart(void);
void _release_duart(void);
void clear_led(int);
void set_led(int);
