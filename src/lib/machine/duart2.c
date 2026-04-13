#include <stdio.h>
#include <machine.h>

struct duart_port {
    volatile uint8_t *mode_regs;
    volatile uint8_t *sr_csr_reg;
    volatile uint8_t *cmd_reg;
    volatile uint8_t *acr_reg;
    volatile uint8_t *data_reg;
    circular_buffer_t rx_buff;
    circular_buffer_t tx_buff;
    uint8_t rts_bit;
};

typedef struct duart_port duart_port_t;

static unsigned int saved_isr = 0;
static duart_port_t channel_a;

#define NOP() for(int i=0; i<1000; i++) { ; }

/* Circular buffers */

/* End of circular buffer code */

/* Channel independent config */
static inline void init_duart_channel(duart_port_t *channel) {
    *channel->cmd_reg = CR_RESET_MR_PTR;
    NOP();
    *channel->cmd_reg = CR_RESET_TX;
    NOP();
    *channel->cmd_reg = CR_SET_EXT_RX;  // Set X bit on xr68c681 for RX
    NOP();
    *channel->cmd_reg = CR_SET_EXT_TX;  // Set X bit on xr68c681 for TX
    NOP();

    *channel->mode_regs = 0x93;         // MR1: Enable Rx RTS, No Parity, 8-bits
    *channel->mode_regs = 0x07;         // MR2: Normal mode, No handshake, Stop bit length=1.000
    *channel->sr_csr_reg = 0x88;        // Baud rate: 230400
    *duart_opr_set = channel->rts_bit;  // Assert RTS
}

static inline void handle_channel_irq(uint8_t interrupt_status_reg, duart_port_t *channel) {
    /* Character received interrupt? */
    if (interrupt_status_reg & ISR_RX_READY) {
        register volatile uint8_t status = *channel->sr_csr_reg;

        if (status & SR_RX_READY) {
            cb_insert(&channel->rx_buff, *channel->data_reg);
            return;
        }
    }

    /* Character transmitted interrupt? */
    if (interrupt_status_reg & ISR_TX_READY) {
        int next_char = cb_remove(&channel->tx_buff);

        if (next_char == -1) {
            // no more chars in buffer
            register volatile uint8_t status = *channel->sr_csr_reg;
            
            if (status & SR_TX_EMPTY)
                *channel->cmd_reg = CR_DISABLE_TX;
        }
        else {
            *channel->data_reg = (unsigned char)next_char;
        }
    }
}

ISR duart_irq_handler() {
    register uint8_t interrupt_status_reg = *duart_isr;

    handle_channel_irq(interrupt_status_reg, &channel_a);
}

static void pre_init_duart(void) {
    channel_a.mode_regs = duart_mr1a;
    channel_a.sr_csr_reg = duart_sra;
    channel_a.cmd_reg = duart_cra;
    channel_a.acr_reg = duart_acr;
    channel_a.data_reg = duart_rba;
    channel_a.rts_bit = 1;

    cb_reset(&channel_a.tx_buff);
    cb_reset(&channel_a.rx_buff);

    *duart_acr = 0x00;  // Baud rate table: set 1
}

void _claim_duart(void) {
}

void ___claim_duart(void) {
    uint8_t duart_vector_number = *duart_ivr;

    INTSOFF();

    pre_init_duart();

    init_duart_channel(&channel_a);

    saved_isr = set_isr_handler(duart_vector_number, (unsigned int)duart_irq_handler);
    *duart_imr = ISR_RX_READY | ISR_TX_READY;
    INTSON();
}

void _release_duart(void) {
}

void ___release_duart(void) {
    uint8_t duart_vector_number = *duart_ivr;   /* Grab the vector number used by the duart */

    set_isr_handler(duart_vector_number, saved_isr);
}

int _buffered_getchar(void) {
    while (cb_is_empty(&channel_a.rx_buff)) {
        ;
    }

    return cb_remove(&channel_a.rx_buff);
}

int _buffered_char_available() {
    return (channel_a.rx_buff.remove != channel_a.rx_buff.insert);
}

void _buffered_putchar(int ch) {
    /* Add it to the TX buffer */
    cb_insert(&channel_a.tx_buff, ch);
    *channel_a.cmd_reg = CR_ENABLE_TX;
}
