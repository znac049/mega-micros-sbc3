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
#include <string.h>
#include <machine.h>
#include <extras.h>

#include "micromon.h"

static kduart_port_t channel_a;
static kduart_port_t channel_b;
static kduart_port_t *channels[] = {&channel_a, &channel_b};
static kduart_t duart;

// The IMR is write-only, so keep a copy of what's in it
static uint8_t imr_shadow;

#define SR_IPL_MASK     0x0700

// Mask all interrupts, returning the old SR
static inline uint16_t irq_off(void) {
    uint16_t sr;

    __asm__ volatile ("move.w %%sr,%0\n\tor.w #0x0700,%%sr" : "=d" (sr) : : "memory", "cc");

    return sr;
}

static inline void irq_restore(uint16_t sr) {
    __asm__ volatile ("move.w %0,%%sr" : : "d" (sr) : "memory", "cc");
}

// Change the IMR (call with interrupts off)
static inline void imr_set(uint8_t bits) {
    imr_shadow |= bits;
    *duart_imr = imr_shadow;
}

static inline void imr_clear(uint8_t bits) {
    imr_shadow &= ~bits;
    *duart_imr = imr_shadow;
}

/*
 * Feed the transmitter from the buffer for as long as it will take
 * characters. Once the buffer is empty, stop asking for TxRDY interrupts -
 * TxRDY stays set while the transmitter is idle, so leaving it enabled
 * would interrupt continuously.
 *
 * Called from the interrupt handler, or with interrupts off.
 */
static void tx_service(kduart_port_t *channel) {
    while ((channel->tx_count > 0) && (*channel->sr_csr_reg & SR_TX_READY)) {
        *channel->data_reg = channel->tx_buff[channel->tx_remove];
        channel->tx_remove = (channel->tx_remove + 1) & (TX_BUFFER_SIZE - 1);
        channel->tx_count--;
    }

    if (channel->tx_count == 0) {
        imr_clear(channel->tx_imr_bit);
    }
}

static void tx_flush(kduart_port_t *channel);

static inline void handle_channel_irq(uint8_t interrupt_status_reg, kduart_port_t *channel) {
    /* Transmitter ready for more? (Only if we asked to be told) */
    if ((interrupt_status_reg & ISR_TX_READY) && (imr_shadow & channel->tx_imr_bit)) {
        tx_service(channel);
    }

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

    // Let anything already queued go at the old speed
    if (channel->tx_irq == YES) {
        tx_flush(channel);
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
    uint32_t baud = 38400;

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
    if (duart.is_xr68c681) {
        baud = (duart.clock_doubled)?230400:115200;
    }

    if (set_baud(channel, baud) == NOT_OK) {
        if (set_baud(channel, 9600) == NOT_OK) {
            *channel->sr_csr_reg = 0x88;
        }
    }

    channel->rx_insert = 0;
    channel->rx_remove = 0;
    channel->rx_count = 0;

    channel->tx_insert = 0;
    channel->tx_remove = 0;
    channel->tx_count = 0;
}

static void init_duart_structures(void) {
    channel_a.mode_regs = duart_mr1a;
    channel_a.sr_csr_reg = duart_sra;
    channel_a.cmd_reg = duart_cra;
    channel_a.acr_reg = duart_acr;
    channel_a.data_reg = duart_rba;
    channel_a.rts_bit = 1;
    channel_a.tx_imr_bit = IMR_TXRDY_A;

    channel_b.mode_regs = duart_mr1b;
    channel_b.sr_csr_reg = duart_srb;
    channel_b.cmd_reg = duart_crb;
    channel_b.acr_reg = duart_acr;
    channel_b.data_reg = duart_rbb;
    channel_b.rts_bit = 2;
    channel_b.tx_imr_bit = IMR_TXRDY_B;
}

static inline void set_counter(uint16_t val) {
    *duart_ctur = (val>>8);
    *duart_ctlr = (val&0xff);
}

static inline uint16_t get_counter(void) {
    register uint16_t hi = *duart_cur;
    register uint16_t lo = *duart_clr;

    return (hi << 8) | lo;
}

static int is_clock_doubled(void) {
    uint8_t x =*duart_stop_counter;
    uint32_t now = ticks();
    uint32_t target = now + 100;

    uint16_t count;

    (void)x;

    set_counter(0);

    x = *duart_start_counter;
    while (ticks() < target) {
        ;
    }

    x = *duart_stop_counter;
    count = (uint16_t)0xffff - get_counter();

    return (count > 44000)?YES:NO; 
}


int duart_clock_doubled(void) {
    return duart.clock_doubled;
}

void early_duart_setup(int is_xr) {
    init_duart_structures();

    duart.is_xr68c681 = is_xr;
    duart.clock_doubled = is_xr;
}

void setup_duart(int is_xr) {
    *duart_acr = 0x30;  // Baud rate table: set 1, counter is clock/16

    duart.is_xr68c681 = is_xr;
    duart.clock_doubled = is_clock_doubled();

    // Interrupt handler
    *duart_ivr = DUART_VECTOR_NUMBER;
    set_isr_handler(DUART_VECTOR_NUMBER, (unsigned int)duart_irq_handler);

    init_duart_channel(&channel_b);
    init_duart_channel(&channel_a);

    {
        uint16_t sr = irq_off();

        // RX interrupts on channel A; TxRDY interrupts get turned on as needed
        imr_shadow = IMR_RXRDY_A;
        *duart_imr = imr_shadow;

        channel_a.tx_irq = YES;
        channel_b.tx_irq = YES;

        irq_restore(sr);
    }
}

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

static int buffered_rx_available(kduart_port_t *channel) {
    return channel->rx_count;
}

static void polled_tx_char(int ch, kduart_port_t *channel) {
    while ((*channel->sr_csr_reg & SR_TX_READY) == 0) {
        ;
    }

    *channel->data_reg = ch;
}

/*
 * Queue a character for the interrupt handler to send.
 *
 * If the buffer is full we feed the transmitter ourselves while waiting,
 * in case our caller is running at a level that keeps the handler out.
 * With every interrupt masked (IPL 7 - at reset, or in a fault handler)
 * the handler can't run at all, so send what's queued and then this
 * character the old way, keeping everything in order.
 */
static void buffered_tx_char(int ch, kduart_port_t *channel) {
    uint16_t sr = irq_off();

    if ((sr & SR_IPL_MASK) == SR_IPL_MASK) {
        while (channel->tx_count > 0) {
            tx_service(channel);
        }

        polled_tx_char(ch, channel);
        irq_restore(sr);

        return;
    }

    while (channel->tx_count >= TX_BUFFER_SIZE) {
        tx_service(channel);

        // Let the interrupt handler in for a moment
        irq_restore(sr);
        sr = irq_off();
    }

    channel->tx_buff[channel->tx_insert] = (uint8_t)ch;
    channel->tx_insert = (channel->tx_insert + 1) & (TX_BUFFER_SIZE - 1);
    channel->tx_count++;

    // Start things off if the transmitter is idle, then let TxRDY drive the rest
    tx_service(channel);

    if (channel->tx_count > 0) {
        imr_set(channel->tx_imr_bit);
    }

    irq_restore(sr);
}

/*
 * Wait until everything queued has been sent and the transmitter has
 * finished with the last character.
 */
static void tx_flush(kduart_port_t *channel) {
    bool_t empty;

    do {
        uint16_t sr = irq_off();

        tx_service(channel);
        empty = (channel->tx_count == 0)?YES:NO;

        irq_restore(sr);
    } while (empty == NO);

    while ((*channel->sr_csr_reg & SR_TX_EMPTY) != SR_TX_EMPTY) {
        ;
    }
}

int bios_getchar(uint8_t port) {
    if (port > 1) {
        return NOT_OK;
    }
    
    return buffered_rx_char(channels[port]);
}

int kgetchar(void) {
    return bios_getchar(0);
}

int bios_putchar(uint8_t port, int c) {
    if (port > 1) {
        return NOT_OK;
    }
    
    if (channels[port]->tx_irq == YES) {
        buffered_tx_char(c, channels[port]);
    }
    else {
        polled_tx_char(c, channels[port]);
    }

    return c;
}

int kputchar(int c) {
    return bios_putchar(0, c);
}

int bios_char_available(uint8_t port) {
    if (port > 1) {
        return NO;
    }
    
    return buffered_rx_available(channels[port]);
}

int kchar_available(void) {
    return bios_char_available(0);
}

char *bios_gets(uint8_t port, char *s) {
    int ch;
    int i = 0;

    if (port > 1) {
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

int bios_puts(uint8_t port, const char *s) {
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

int bios_printf(uint8_t port, const char *format, ...) {
	int res;
	va_list args;
	char buffer[512];

	va_start(args, format);
	res = vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);

	bios_puts(port, buffer);

	return res;
}

int bios_set_baud(uint8_t port, uint32_t baudrate) {
    if (port > 1) {
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

int bios_flush(uint8_t port) {
    if (port > 1) {
        return -1;
    }

    tx_flush(channels[port]);

    return 0;
}

static int handles_path(const char *pathname) {
    if ((strcasecmp(pathname, "//ser1") == 0) || (strcasecmp(pathname, "//ser2") == 0)) {
        return YES;
    }

    return NO;
}

static int duart_isatty(uint8_t port) {
    if (port > 1) {
        return NOT_OK;
    }

    
    return YES;
}
int setup_vfs_duart_handler(vfs_fs_t *vfs) {
    if (vfs == NULL) {
        return NOT_OK;
    }

    vfs->type = VFS_TYPE_CHAR;
    vfs->handles_path = handles_path;
    vfs->name = "68681 duart";

    vfs->api.chardev.putchar = bios_putchar;
    vfs->api.chardev.getchar = bios_getchar;
    vfs->api.chardev.char_available = bios_char_available;
    vfs->api.chardev.flush = bios_flush;
    vfs->api.chardev.isatty = duart_isatty;

    return OK;
}
