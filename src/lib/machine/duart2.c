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

#undef  RX_INTS
#undef  TX_INTS

#define XR68681 1            /* We're using the enhanced duart */

#if defined(RX_INTS) || defined(TX_INTS)
static unsigned int saved_isr = 0;
#endif

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
    // *channel->cmd_reg = CR_RESET_ERROR;
    // NOP();
    // *channel->cmd_reg = CR_RESET_BREAK;
    // NOP();

#if XR68681
    *channel->cmd_reg = CR_SET_EXT_RX;  // Set X bit on xr68c681 for RX
    NOP();
    *channel->cmd_reg = CR_SET_EXT_TX;  // Set X bit on xr68c681 for TX
    NOP();
#endif

    *channel->cmd_reg = CR_ENABLE_RX;
    NOP();

#if !defined(TX_INTS)
    *channel->cmd_reg = CR_ENABLE_TX;
    NOP();
#endif

    // RTS Rx handshaking is taken care of in code, not hardware
    *channel->mode_regs = 0x93;         // MR1: RX Handshake, No Parity, 8-bits
    *channel->mode_regs = 0x07;         // MR2: Normal mode, No handshake, Stop bit length=1.000
    *channel->sr_csr_reg = 0x88;        // Baud rate: 230400

    *duart_opr_set = channel->rts_bit;  // Assert RTS
}

static inline void handle_channel_irq(uint8_t interrupt_status_reg, duart_port_t *channel) {
#if defined(RX_INTS)
    /* Character received interrupt? */
    if (interrupt_status_reg & ISR_RX_READY) {
        pit_set_bits_a(1);
        register volatile uint8_t status = *channel->sr_csr_reg;

        if (status & SR_RX_READY) {
            uint8_t ch = *channel->data_reg;

            if (_buf_is_full(&channel->rx_buff)) {
                // This shouldn't happen - yeah, right!
                channel->rx_overruns++;
                *duart_opr_reset = channel->rts_bit;        // Deassert CTS
            }
            else {            
                _buf_put_char(&channel->rx_buff, ch);

                // Handshake ?
                if (_buf_free_space(&channel->rx_buff) < 10) {
                    *duart_opr_reset = channel->rts_bit;    // Deassert CTS
                }
            }
        }
    }
#endif

#if defined(TX_INTS)
    /* Character transmitted interrupt? */
    if (interrupt_status_reg & ISR_TX_READY) {
        uint8_t ch = _buf_get_char(&channel->tx_buff);

        pit_set_bits_a(2);
        if (ch == -1) {
            // no more chars in buffer
            register volatile uint8_t status = *channel->sr_csr_reg;

            if (status & SR_TX_EMPTY) {
                *channel->cmd_reg = CR_DISABLE_TX;
            }
        }
        else {
            *channel->data_reg = ch;
        }
    }
#endif
}

#if defined(RX_INTS) || defined(TX_INTS)
ISR duart_irq_handler() {
    register uint8_t interrupt_status_reg = *duart_isr;

    handle_channel_irq(interrupt_status_reg, &channel_a);
    handle_channel_irq(interrupt_status_reg >> 4, &channel_b);
}
#endif

static void pre_init_duart(void) {
    channel_a.mode_regs = duart_mr1a;
    channel_a.sr_csr_reg = duart_sra;
    channel_a.cmd_reg = duart_cra;
    channel_a.acr_reg = duart_acr;
    channel_a.data_reg = duart_rba;
    channel_a.rts_bit = 1;
    channel_a.rx_overruns = 0;

    channel_b.mode_regs = duart_mr1b;
    channel_b.sr_csr_reg = duart_srb;
    channel_b.cmd_reg = duart_crb;
    channel_b.acr_reg = duart_acr;
    channel_b.data_reg = duart_rbb;
    channel_b.rts_bit = 2;
    channel_b.rx_overruns = 0;

#if defined(RX_INTS)
    _buf_init(&channel_a.rx_buff, 0);
    _buf_init(&channel_b.rx_buff, 0);
#endif

#if defined(TX_INTS)
    _buf_init(&channel_a.tx_buff, 0);
    _buf_init(&channel_b.tx_buff, 0);
#endif

    *duart_acr = 0x00;  // Baud rate table: set 1
}

void _claim_duart(void) {
    uint16_t saved_sr;
#if defined(RX_INTS) || defined(TX_INTS)
    uint8_t duart_vector_number = *duart_ivr;   /* Grab the vector number used by the duart */
#endif

    *pit_paddr = 0xff;  // All outputs
    *pit_pbddr = 0xff;  // All outputs

#if 1
    pit_set_a(255);
    pit_set_b(255);

    idle_for_ticks(400);

    pit_set_a(0);
    pit_set_b(0);
#endif

    pre_init_duart();

    LOCK(saved_sr);

    init_duart_channel(&channel_a);
    init_duart_channel(&channel_b);

    xr68681_device.chardev.getchar = xr68681_getchar;
    xr68681_device.chardev.char_available = xr68681_char_available;
    xr68681_device.chardev.putchar = xr68681_putchar;
    xr68681_device.chardev.flush = xr68681_flush;

#if defined(RX_INTS) || defined(TX_INTS)
    saved_isr = set_isr_handler(duart_vector_number, (unsigned int)duart_irq_handler);
    *duart_imr = 0 
#  if defined(RX_INTS)
                | ISR_RX_READY
#  endif
#  if defined(TX_INTS)
                 | ISR_TX_READY
#  endif
                ;
#endif

    UNLOCK(saved_sr);
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
    uint16_t saved_sr;
#if defined(RX_INTS) || defined(TX_INTS)
    uint8_t duart_vector_number = *duart_ivr;   /* Grab the vector number used by the duart */
#endif

    LOCK(saved_sr);

    *duart_imr = 0;

    release_duart_channel(&channel_a);
    release_duart_channel(&channel_b);

#if defined(RX_INTS) || defined(TX_INTS)
    set_isr_handler(duart_vector_number, saved_isr);
#endif

    UNLOCK(saved_sr);
}

int buffered_rx_char(duart_port_t *channel) {
    int ch;
    lock_state_t saved_sr;

    LOCK(saved_sr);

    while (_buf_is_empty(&channel->rx_buff)) {
        __asm volatile ("");
    }

    ch = _buf_get_char(&channel->rx_buff);

    if (_buf_free_space(&channel->rx_buff) > 100) {
        *duart_opr_set = channel->rts_bit;  // Assert CTS
    }

    UNLOCK(saved_sr)

    return ch;
}

int buffered_rx_available(duart_port_t *channel) {
    int res;
    lock_state_t saved_sr;

    LOCK(saved_sr);

    res = !_buf_is_empty(&channel->rx_buff);

    UNLOCK(saved_sr)

    return res;
}

void buffered_tx_char(int ch, duart_port_t *channel) {
    lock_state_t saved_sr;

    LOCK(saved_sr);
    
    // WTF is this for?
    // for (int i=0; _buf_is_full(&channel->tx_buff) && i<10000; i++) {
    //     __asm volatile("");
    // }

    while (_buf_is_full(&channel->tx_buff)) {
        ;
    }

    _buf_put_char(&channel->tx_buff, ch);

    *channel->cmd_reg = CR_ENABLE_TX;

    UNLOCK(saved_sr);
}

void buffered_flush(duart_port_t *channel) {
    while (!_buf_is_empty(&channel->tx_buff)) {
        ;
    }
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

#if defined(RX_INTS)
    return buffered_rx_char(channels[minor]);
#else
    return safe_rx_char(channels[minor]);
#endif
}

int xr68681_char_available(uint8_t minor) {
    if (minor >= 2)
        return -1;

#if defined(RX_INTS)
    return buffered_rx_available(channels[minor]);
#else
    return safe_rx_available(channels[minor]);
#endif
}

void xr68681_putchar(int ch, uint8_t minor) {
    if (minor >= 2)
        return;

#if defined(TX_INTS)
    buffered_tx_char(ch, channels[minor]);
#else
    safe_tx_char(ch, channels[minor]);
#endif

    pit_set_b(ch);
}

int xr68681_flush(uint8_t minor) {
    if (minor >= 2)
        return -1;

#if defined(TX_INTS)
    buffered_flush(channels[minor]);
#else
    safe_flush(channels[minor]);
#endif

    return 0;
}