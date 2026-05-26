#include <stdio.h>
#include <fcntl.h>
#include <machine.h>

static unsigned int saved_isr = 0;
static duart_port_t channel_a;
static duart_port_t channel_b;
static duart_port_t *channels[] = {&channel_a, &channel_b};

#define NOP() for(int i=0; i<1000; i++) { ; }

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
    handle_channel_irq(interrupt_status_reg, &channel_b);
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

    cb_reset(&channel_a.tx_buff);
    cb_reset(&channel_a.rx_buff);

    cb_reset(&channel_b.tx_buff);
    cb_reset(&channel_b.rx_buff);

    *duart_acr = 0x00;  // Baud rate table: set 1
}

void _claim_duart(void) {
    uint8_t duart_vector_number = *duart_ivr;   /* Grab the vector number used by the duart */

    pre_init_duart();

    INTSOFF();

    init_duart_channel(&channel_a);
    init_duart_channel(&channel_b);

    xr68681_device.chardev.getchar = xr68681_getchar;
    xr68681_device.chardev.char_available = xr68681_char_available;
    xr68681_device.chardev.putchar = xr68681_putchar;
    xr68681_device.chardev.flush = xr68681_flush;

    saved_isr = set_isr_handler(duart_vector_number, (unsigned int)duart_irq_handler);
    *duart_imr = ISR_RX_READY | ISR_TX_READY;

    INTSON();
}

void _release_duart(void) {
    uint8_t duart_vector_number = *duart_ivr;   /* Grab the vector number used by the duart */

    INTSOFF();

    set_isr_handler(duart_vector_number, saved_isr);

    INTSON();

#if 0
    printf("\nDuart Ch.a:\n");
    printf("  SR: 0x%08x\n", channel_a.sr_csr_reg);
    printf("  DR: 0x%08x\n", channel_a.data_reg);

    printf("\nDuart Ch.a:\n");
    printf("  SR: 0x%08x\n", channel_b.sr_csr_reg);
    printf("  DR: 0x%08x\n", channel_b.data_reg);
#endif
}

int buffered_rx_char(duart_port_t *channel) {
    while (cb_is_empty(&channel->rx_buff)) {
        ;
    }

    return cb_remove(&channel->rx_buff);
}

int buffered_rx_available(duart_port_t *channel) {
    return (!cb_is_empty(&channel->rx_buff));
}

void buffered_tx_char(int ch, duart_port_t *channel) {
    /* Add it to the TX buffer */
    cb_insert(&channel->tx_buff, ch);
    *channel->cmd_reg = CR_ENABLE_TX;
}

#if 0
int polled_rx_char(duart_port_t *channel) {
    uint8_t status = *channel->sr_csr_reg;
    uint8_t ch;

    while ((status && SR_RX_READY) == 0) {
       status = *channel->sr_csr_reg; 
    }

    ch = *channel->data_reg;

    return (int)ch;
}

int polled_rx_available(duart_port_t *channel) {
    uint8_t status = *channel->sr_csr_reg;

    return (status & SR_RX_READY)?1:0;
}

void polled_tx_char(int ch, duart_port_t *channel) {
    uint8_t status = *channel->sr_csr_reg;

    while ((status && SR_TX_READY) == 0) {
       status = *channel->sr_csr_reg; 
    }

    *channel->data_reg = ch;
}
#endif

#if 0
int _polled_getchar(void) {
    return polled_rx_char(&channel_a);
}

int _polled_char_available(void) {
    return polled_rx_available(&channel_a);    
}

void _polled_putchar(int ch) {
    polled_tx_char(ch, &channel_a);
}

#endif 
/* io_device functions */
int xr68681_getchar(uint8_t minor) {
    if (minor >= 2)
        return -1;

    return buffered_rx_char(channels[minor]);
}

int xr68681_char_available(uint8_t minor) {
    if (minor >= 2)
        return -1;

    return buffered_rx_available(channels[minor]);
}

void xr68681_putchar(int ch, uint8_t minor) {
    if (minor >= 2)
        return;

    buffered_tx_char(ch, channels[minor]);
}

int xr68681_flush(uint8_t minor) {
    return 0;
}