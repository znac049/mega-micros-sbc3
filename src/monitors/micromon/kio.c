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

#define XR68681 1            /* We're using the enhanced duart */

struct kduart_port {
    volatile uint8_t *mode_regs;
    volatile uint8_t *sr_csr_reg;
    volatile uint8_t *cmd_reg;
    volatile uint8_t *acr_reg;
    volatile uint8_t *data_reg;
    uint8_t rts_bit;
};

typedef struct kduart_port kduart_port_t;

static kduart_port_t channel_a;
static kduart_port_t channel_b;
static kduart_port_t *channels[] = {&channel_a, &channel_b};


/* Channel independent config */
static inline void init_duart_channel(kduart_port_t *channel) {
    *channel->cmd_reg = CR_RESET_MR_PTR;
    NOP();
    *channel->cmd_reg = CR_RESET_TX;
    NOP();
    *channel->cmd_reg = CR_RESET_RX;
    NOP();

#if XR68681
    *channel->cmd_reg = CR_SET_EXT_RX;  // Set X bit on xr68c681 for RX
    NOP();
    *channel->cmd_reg = CR_SET_EXT_TX;  // Set X bit on xr68c681 for TX
    NOP();
#endif

    *channel->cmd_reg = CR_ENABLE_RX;
    NOP();
    *channel->cmd_reg = CR_ENABLE_TX;
    NOP();

    // RTS Rx handshaking is taken care of in code, not hardware
    *channel->mode_regs = 0x93;         // MR1: RX Handshake, No Parity, 8-bits
    *channel->mode_regs = 0x07;         // MR2: Normal mode, No handshake, Stop bit length=1.000
    *channel->sr_csr_reg = 0x88;        // Baud rate: 230400

    *duart_opr_set = channel->rts_bit;  // Assert RTS
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

    *duart_acr = 0x00;  // Baud rate table: set 1
}

void setup_duart(void) {
    init_duart_structures();

    init_duart_channel(&channel_a);
    init_duart_channel(&channel_b);
}

static int safe_rx_char(kduart_port_t *channel) {
    while ((*channel->sr_csr_reg & SR_RX_READY) == 0) {
        ;
    }

    return *channel->data_reg;
}

static int safe_rx_available(kduart_port_t *channel) {
    uint8_t status = *channel->sr_csr_reg;

    return (status & SR_RX_READY)?1:0;
}

static void safe_tx_char(int ch, kduart_port_t *channel) {
    while ((*channel->sr_csr_reg & SR_TX_READY) == 0) {
        ;
    }

    *channel->data_reg = ch;
}

static void safe_flush(kduart_port_t *channel) {
    // Wait for any TX operations that are already in progress to complete

    while ((*channel->sr_csr_reg & SR_TX_EMPTY) != SR_TX_EMPTY) {
        ;
    }
}

int kgetchar(void) {
    return safe_rx_char(channels[0]);
}

int kputchar(int c) {
    safe_tx_char(c, channels[0]);

    return c;
}

bool_t kchar_available(void) {
    return safe_rx_available(channels[0]);
}

char *kgets(char *s) {
    int ch;
    int i = 0;

    while ((ch = kgetchar()) != -1) {
        switch(ch) {
            case '\r': case '\n':
                s[i] = EOS;
                kputchar('\n');

                return s;

            case BS:
                if (i) {
                    kputchar(BS);
                    kputchar(' ');
                    kputchar(BS);
                    i--;
                }
                break;

            default:
                kputchar(ch);
                s[i++] = ch;
                break;
        }
    }

    return NULL;
}

int kputs(const char *s) {
    while (*s) {
        kputchar(*s++);
    }

    return 0;
}

int kprintf(const char *format, ...) {
	int res;
	va_list args;
	char buffer[512];

	va_start(args, format);
	res = vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);

	kputs(buffer);

	return res;
}