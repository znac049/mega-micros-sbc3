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

#include <ctype.h>
#include <stddef.h>
#include <machine.h>

#include "micromon.h"

#define RX_INTS                     // Rx interrupts

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

static kduart_port_t channel_a;
static kduart_port_t channel_b;
static kduart_port_t *channels[] = {&channel_a, &channel_b};
static kduart_t duart;

static inline void handle_channel_irq(uint8_t interrupt_status_reg, kduart_port_t *channel) {
    /* Character received interrupt? */
    if (interrupt_status_reg & ISR_RX_READY) {
        register volatile uint8_t status = *channel->sr_csr_reg;

        // if (status & SR_RX_READY) {
            uint8_t ch = *channel->data_reg;

           // Any of the error bits set?
            if (status & 0xf0) {
                // discard the character and set an error led
                *duart_opr_set = 0x40;
            }
            else {
                // Is there room in the buffer?
                if (channel->rx_count >= RX_BUFFER_SIZE) {
                    // The buffer is already full. This should only happen if the sending
                    // side has ignored our RTS signal. Set an error led and drop the
                    // received char on the floor
                    *duart_opr_set = 0x40;
                    // *pit_pbdr = 0;
                }
                else {
                    // *pit_pbdr = ~ch;

                    channel->rx_buff[channel->rx_insert] = ch;
                    channel->rx_insert++;

                    if (channel->rx_insert >= RX_BUFFER_SIZE) {
                        channel->rx_insert = 0;
                    }

                    channel->rx_count++;

                    // Check that the buffer isn't filling up
                    if ((channel->rx_count == HIGH_WATER_MARK) && (channel->rts_asserted == YES)) {
                        // Deassert RTS
                        *duart_opr_reset = channel->rts_bit;
                        channel->rts_asserted = NO;
                    }

                    // *pit_padr = ~channel->rx_count;
                }
            }
        // }
    }
}

static ISR duart_irq_handler (void) {
    register uint8_t interrupt_status_reg = *duart_isr;

    handle_channel_irq(interrupt_status_reg, &channel_a);
    handle_channel_irq(interrupt_status_reg >> 4, &channel_b);
}

// The first two rows work on all 68681 variants, but the last two only work
// on the Exar xr68c681
static uint32_t baud_rate_table[4][13] = {
    {50, 110, 134, 200, 300,  600,   1200,  1050,  2400,   4800, 7200, 9600, 38400},
    {75, 110, 134, 150, 300,  600,   1200,  2000,  2400,   4800, 1800, 9600, 19200},
    {75, 110, 134, 150, 3600, 14400, 28800, 57600, 115200, 4800, 1800, 9600, 19200},
    {50, 110, 134, 200, 3600, 14400, 28800, 57600, 115200, 4800, 7200, 9600, 38400},
};

static int find_baudrate(uint32_t baud, uint32_t *table) {
    for (int i=0; i<13; i++) {
        if (table[i] == baud) {
            return i;
        }
    }

    return -1;
}

static int set_baud(kduart_port_t *channel, uint32_t baud) {
    int table = 0;
    uint32_t br = baud;
    int table_offset;
    uint8_t acr = 0;
    int extended = NO;

    if (duart.clock_doubled) {
        br = br >> 1;
    }

    table_offset = find_baudrate(br, baud_rate_table[table]);
    if (table_offset == -1) {
        acr = 0x80;
        table_offset = find_baudrate(br, baud_rate_table[++table]);
    }

    if (table_offset == -1 && duart.is_xr68c681) {
        extended = YES;
        acr = 0;

        table_offset = find_baudrate(br, baud_rate_table[++table]);

        if (table_offset == -1) {
            acr = 0x80;
            table_offset = find_baudrate(br, baud_rate_table[++table]);
        }
    }

    if (table_offset == -1) {
        return NOT_OK;
    }

    if (channel != &channel_b) {
        bios_printf(1, "SB(%d): X=%d, ACR=$%02x, CSR=$%02x\n", baud, extended, acr, (table_offset<<4)|table_offset);
    }

    if (extended == YES) {
        *channel->cmd_reg = CR_SET_EXT_RX;
        NOP();
        *channel->cmd_reg = CR_SET_EXT_TX;
        NOP();
    }   
    else {
        *channel->cmd_reg = CR_CLEAR_EXT_RX;
        NOP();
        *channel->cmd_reg = CR_CLEAR_EXT_TX;
        NOP();
    }

    // Select baud rate table 0 or 1
    *channel->acr_reg = acr | ACR_CK_DIV_16;
    *channel->sr_csr_reg = (table_offset << 4) | table_offset;  // Same for Tx and Rx

    return OK;
}

/* Channel independent config */
static inline void init_duart_channel(kduart_port_t *channel) {
    *channel->cmd_reg = CR_RESET_MR_PTR;
    NOP();
    *channel->cmd_reg = CR_RESET_TX;
    NOP();
    *channel->cmd_reg = CR_RESET_RX;
    NOP();

    // if (duart.is_xr68c681 == YES) {
    //     *channel->cmd_reg = CR_SET_EXT_RX;  // Set X bit on xr68c681 for RX
    //     NOP();
    //     *channel->cmd_reg = CR_SET_EXT_TX;  // Set X bit on xr68c681 for TX
    //     NOP();
    // }

    // RTS Rx handshaking is taken care of in code, not hardware
    *channel->mode_regs = 0x13;         // MR1: No RX Handshake, No Parity, 8-bits
    *channel->mode_regs = 0x07;         // MR2: Normal mode, No Tx handshake, Stop bit length=1.000

    *channel->cmd_reg = CR_ENABLE_RX;
    NOP();
    *channel->cmd_reg = CR_ENABLE_TX;
    NOP();

    *duart_opr_set = channel->rts_bit;  // Assert RTS
    channel->rts_asserted = YES;

    // *channel->sr_csr_reg = 0x88;        // Baud rate: 230400
    if (set_baud(channel, 230400) == NOT_OK) {
        if (set_baud(channel, 9600) == NOT_OK) {
            *channel->sr_csr_reg = 0x88;
        }
    }

#ifdef RX_INTS
    channel->rx_insert = 0;
    channel->rx_remove = 0;
    channel->rx_count = 0;
#endif
}

static void init_duart_structures(void) {
    channel_a.mode_regs = duart_mr1a;
    channel_a.sr_csr_reg = duart_sra;
    channel_a.cmd_reg = duart_cra;
    channel_a.acr_reg = duart_acr;
    channel_a.data_reg = duart_rba;
    channel_a.rts_bit = 1;

    channel_b.mode_regs = duart_mr1b;
    channel_b.sr_csr_reg = duart_srb;
    channel_b.cmd_reg = duart_crb;
    channel_b.acr_reg = duart_acr;
    channel_b.data_reg = duart_rbb;
    channel_b.rts_bit = 2;
}

void setup_duart(int is_xr, int clk_dbl) {
    init_duart_structures();

    *duart_acr = 0x00;  // Baud rate table: set 1

    duart.is_xr68c681 = is_xr;
    duart.clock_doubled = clk_dbl;

    // Interrupt handler
    *duart_ivr = DUART_VECTOR_NUMBER;
    set_isr_handler(DUART_VECTOR_NUMBER, (unsigned int)duart_irq_handler);

    init_duart_channel(&channel_b);
    init_duart_channel(&channel_a);

#ifdef RX_INTS
    *duart_imr = 0x02;                  //  RX interrupts, channel A
#endif

    // *pit_padr = 0;
    // *pit_pbdr = 0;
}
#ifndef RX_INTS
static int polled_rx_char(kduart_port_t *channel) {
    while ((*channel->sr_csr_reg & SR_RX_READY) == 0) {
        ;
    }

    return *channel->data_reg;
}
#else
static int buffered_rx_char(kduart_port_t *channel) {
    uint16_t sr;
    int ch;

    // Wait for a character to appear in the buffer
     while (channel->rx_count == 0) {
        ;
     }

     LOCK(sr);

     ch = channel->rx_buff[channel->rx_remove];

     channel->rx_remove++;
     if (channel->rx_remove >= RX_BUFFER_SIZE) {
        channel->rx_remove = 0;
     }

     // Do we need to assert RTS?
     channel->rx_count--;
     if ((channel->rx_count == LOW_WATER_MARK) && (channel->rts_asserted == NO)) {
        // ReAssert RTS
        *duart_opr_set = channel->rts_bit;
        channel->rts_asserted = YES;
     }

    //  *pit_padr = ~channel->rx_count;

     UNLOCK(sr);

     return ch;
}
#endif

#ifndef RX_INTS
static int polled_rx_available(kduart_port_t *channel) {
    uint8_t status = *channel->sr_csr_reg;

    return (status & SR_RX_READY)?1:0;
}
#else
static int buffered_rx_available(kduart_port_t *channel) {
    return channel->rx_count;
}
#endif

static void polled_tx_char(int ch, kduart_port_t *channel) {
    while ((*channel->sr_csr_reg & SR_TX_READY) == 0) {
        ;
    }

    *channel->data_reg = ch;
}

static void polled_flush(kduart_port_t *channel) {
    // Wait for any TX operations that are already in progress to complete

    while ((*channel->sr_csr_reg & SR_TX_EMPTY) != SR_TX_EMPTY) {
        ;
    }
}

int bios_getchar(int port) {
    if ((port < 0) || (port > 1)) {
        return NOT_OK;
    }
    
#ifdef RX_INTS
    return buffered_rx_char(channels[port]);
#else
    return polled_rx_char(channels[port]);
#endif
}

int kgetchar(void) {
    return bios_getchar(0);
}

int bios_putchar(int port, int c) {
    if ((port < 0) || (port > 1)) {
        return NOT_OK;
    }
    
    polled_tx_char(c, channels[port]);

    return c;
}

int kputchar(int c) {
    return bios_putchar(0, c);
}

bool_t bios_char_available(int port) {
    if ((port < 0) || (port > 1)) {
        return NO;
    }
    
#ifdef RX_INTS
    return buffered_rx_available(channels[port]);
#else
    return polled_rx_available(channels[port]);
#endif
}

bool_t kchar_available(void) {
    return bios_char_available(0);
}

char *bios_gets(int port, char *s) {
    int ch;
    int i = 0;

    if ((port <0) || (port > 1)) {
        return NULL;
    }

    while ((ch = bios_getchar(port)) != -1) {
        switch(ch) {
            case '\r': case '\n':
                s[i] = EOS;
                bios_putchar(port, '\n');

                return s;

            case BS:
                if (i) {
                    bios_putchar(port, BS);
                    bios_putchar(port, ' ');
                    bios_putchar(port, BS);
                    i--;
                }
                break;

            default:
                bios_putchar(0, ch);
                s[i++] = ch;
                break;
        }
    }

    return NULL;
}

char *kgets(char *s) {
    return bios_gets(0, s);
}

int bios_puts(int port, const char *s) {
    while (*s) {
        bios_putchar(port, *s++);
    }

    return 0;
}

int kputs(const char *s) {
    return bios_puts(0, s);
}

int kprintf(const char *format, ...) {
	int res;
	va_list args;
	char buffer[512];

	va_start(args, format);
	res = vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);

	bios_puts(0, buffer);

	return res;
}

int bios_printf(int port, const char *format, ...) {
	int res;
	va_list args;
	char buffer[512];

	va_start(args, format);
	res = vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);

	bios_puts(port, buffer);

	return res;
}

int bios_set_baud(int port, uint32_t baudrate) {
    if ((port < 0) || (port > 1)) {
        return NOT_OK;
    }

    return set_baud(channels[port], baudrate);
}

int kio_rx_info(void) {
    bios_printf(1, "0: ins=%d, rem=%d, #=%d, rts=%d    ", 
        channel_a.rx_insert, channel_a.rx_remove, channel_a.rx_count, channel_a.rts_asserted);   
    bios_printf(1, "1: ins=%d, rem=%d, #=%d, rts=%d\n", 
        channel_b.rx_insert, channel_b.rx_remove, channel_b.rx_count, channel_b.rts_asserted);

    return channel_a.rx_count;
}