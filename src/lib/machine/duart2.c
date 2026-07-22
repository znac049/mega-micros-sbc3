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

#include <stdio.h>
#include <fcntl.h>
#include <machine.h>

#define XR68681 1            /* We're using the enhanced duart */

static duart_port_t channel_a;
static duart_port_t channel_b;
static duart_port_t *channels[] = {&channel_a, &channel_b};

system_io_device_t xr68681_device;

// Forward declarations
int xr68681_getchar(uint8_t minor);
int xr68681_char_available(uint8_t minor);
void xr68681_putchar(int ch, uint8_t minor);
int xr68681_flush(uint8_t minor);

/* Channel independent config */
static inline void init_duart_channel(duart_port_t *channel) {
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

static void pre_init_duart(void) {
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

void _claim_duart(void) {
    pre_init_duart();

    // init_duart_channel(&channel_a);
    // init_duart_channel(&channel_b);

    xr68681_device.chardev.getchar = xr68681_getchar;
    xr68681_device.chardev.char_available = xr68681_char_available;
    xr68681_device.chardev.putchar = xr68681_putchar;
    xr68681_device.chardev.flush = xr68681_flush;
}

static inline void release_duart_channel(duart_port_t *channel) {
    *channel->cmd_reg = CR_RESET_MR_PTR;
    NOP();
    *channel->cmd_reg = CR_RESET_TX;
    NOP();
    *channel->cmd_reg = CR_RESET_RX;
    NOP();

    *channel->cmd_reg = CR_SET_EXT_RX;  // Set X bit on xr68c681 for RX
    NOP();
    *channel->cmd_reg = CR_SET_EXT_TX;  // Set X bit on xr68c681 for TX
    NOP();

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

void _release_duart(void) {
    // release_duart_channel(&channel_a);
    // release_duart_channel(&channel_b);
}

int safe_rx_char(duart_port_t *channel) {
    while ((*channel->sr_csr_reg & SR_RX_READY) == 0) {
        ;
    }

    return *channel->data_reg;
}

int safe_rx_available(duart_port_t *channel) {
    uint8_t status = *channel->sr_csr_reg;

    return (status & SR_RX_READY)?1:0;
}

void safe_tx_char(int ch, duart_port_t *channel) {
    while ((*channel->sr_csr_reg & SR_TX_READY) == 0) {
        ;
    }

    *channel->data_reg = ch;
}

void safe_flush(duart_port_t *channel) {
    // Wait for any TX operations that are already in progress to complete

    while ((*channel->sr_csr_reg & SR_TX_EMPTY) != SR_TX_EMPTY) {
        ;
    }
}

/* io_device functions */
int xr68681_getchar(uint8_t minor) {
    if (minor >= 2)
        return -1;

    return safe_rx_char(channels[minor]);
}

int xr68681_char_available(uint8_t minor) {
    if (minor >= 2)
        return -1;

    return safe_rx_available(channels[minor]);
}

void xr68681_putchar(int ch, uint8_t minor) {
    if (minor >= 2)
        return;

    safe_tx_char(ch, channels[minor]);
}

int xr68681_flush(uint8_t minor) {
    if (minor >= 2)
        return -1;

    safe_flush(channels[minor]);

    return 0;
}