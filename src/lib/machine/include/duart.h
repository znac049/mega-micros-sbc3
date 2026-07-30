/* 
MIT License

Copyright (c) 2026 Bob Green

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>
#include <machine.h>

/* Duart Register Base Addresses */
#define duart_base ((volatile uint8_t*) 0xad0001)

#define duart_mr1a          ((volatile uint8_t*) duart_base)
#define duart_mr2a          ((volatile uint8_t*) duart_base)
#define duart_sra           ((volatile uint8_t*) duart_base+2)
#define duart_csra          ((volatile uint8_t*) duart_base+2)
#define duart_cra           ((volatile uint8_t*) duart_base+4)
#define duart_rba           ((volatile uint8_t*) duart_base+6)
#define duart_tba           ((volatile uint8_t*) duart_base+6)
#define duart_ipcr          ((volatile uint8_t*) duart_base+8)
#define duart_acr           ((volatile uint8_t*) duart_base+8)
#define duart_isr           ((volatile uint8_t*) duart_base+10)
#define duart_imr           ((volatile uint8_t*) duart_base+10)
#define duart_cur           ((volatile uint8_t*) duart_base+12)
#define duart_ctur          ((volatile uint8_t*) duart_base+12)
#define duart_clr           ((volatile uint8_t*) duart_base+14)
#define duart_ctlr          ((volatile uint8_t*) duart_base+14)
#define duart_mr1b          ((volatile uint8_t*) duart_base+16)
#define duart_mr2b          ((volatile uint8_t*) duart_base+16)
#define duart_srb           ((volatile uint8_t*) duart_base+18)
#define duart_csrb          ((volatile uint8_t*) duart_base+18)
#define duart_crb           ((volatile uint8_t*) duart_base+20)
#define duart_rbb           ((volatile uint8_t*) duart_base+22)
#define duart_tbb           ((volatile uint8_t*) duart_base+22)
#define duart_ivr           ((volatile uint8_t*) duart_base+24)
#define duart_ip            ((volatile uint8_t*) duart_base+26)
#define duart_opcr          ((volatile uint8_t*) duart_base+26)
#define duart_start_counter ((volatile uint8_t*) duart_base+28)
#define duart_opr_set       ((volatile uint8_t*) duart_base+28)
#define duart_stop_counter  ((volatile uint8_t*) duart_base+30)
#define duart_opr_reset     ((volatile uint8_t*) duart_base+30)

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

#define ACR_CK_DIV_16 0x30

#if defined(BAREMETAL)

#define DUART_VECTOR_NUMBER 64
#define RX_BUFFER_SIZE 16
#define HIGH_WATER_MARK 10
#define LOW_WATER_MARK 2


struct kduart_port {
    volatile uint8_t *mode_regs;
    volatile uint8_t *sr_csr_reg;
    volatile uint8_t *cmd_reg;
    volatile uint8_t *acr_reg;
    volatile uint8_t *data_reg;
    uint8_t rts_bit;

    volatile uint8_t rx_buff[RX_BUFFER_SIZE];
    volatile uint8_t rx_insert;
    volatile uint8_t rx_remove;
    volatile uint8_t rx_count;
    volatile unsigned int rts_asserted : 1;
};

typedef struct kduart_port kduart_port_t;

struct kduart {
    unsigned int is_xr68c681 : 1;
    unsigned int clock_doubled : 1;
};

typedef struct kduart kduart_t;


void setup_duart(int is_xr);

int kchar_available(void);
int kgetchar(void);
char *kgets(char *s);
int kputchar(int c);
int kputs(const char *s);
int kprintf(const char *format, ...);
int kgetchar(void);
int kputchar(int c);

int bios_getchar(uint8_t port);
int bios_putchar(uint8_t port, int c);
int bios_char_available(uint8_t port);
char *bios_gets(uint8_t port, char *s);
int bios_puts(uint8_t port, const char *s);
int bios_printf(uint8_t port, const char *format, ...);
int bios_set_baud(uint8_t port, uint32_t baudrate);

int kio_rx_info(void);
int duart_clock_doubled(void);

int setup_vfs_duart_handler(vfs_handler_t *vfs);

#endif