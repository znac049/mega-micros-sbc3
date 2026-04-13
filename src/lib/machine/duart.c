#include <stdio.h>
#include <machine.h>

#define RX_INTS 1
#define TX_INTS 0
#define WITH_INTS (RX_INTS || TX_INTS)

#if WITH_INTS
static unsigned int saved_duart_isr;
#endif

#if RX_INTS
static struct circular_buffer rxbuff_a;
static uint8_t rx=0;
#endif

#if TX_INTS
static struct circular_buffer txbuff_a;
static uint8_t tx=0;
#endif

#if RX_INTS
#endif

static inline bool_t cb_is_empty(struct circular_buffer *cb) {
    return cb->insert == cb->remove;
}

static inline bool_t cb_is_full(struct circular_buffer *cb) {
    uint16_t ins = cb->insert+1;

    if (ins >= CIRCULAR_BUFFER_SIZE) {
        ins = 0;
    }

    return ins == cb->remove;
}

static inline void cb_insert(struct circular_buffer *cb, uint8_t ch) {
    cb->buffer[cb->insert++] = ch;

    if (cb->insert >= CIRCULAR_BUFFER_SIZE) {
        cb->insert = 0;
    }
}

static inline int cb_remove(struct circular_buffer *cb) {
    int res;

    if (cb_is_empty(cb)) {
        return -1;
    }

    res = cb->buffer[cb->remove++];

    if (cb->remove >= CIRCULAR_BUFFER_SIZE) {
        cb->remove = 0;
    }

    return res;
}

static inline void enable_tx_a(void) {
    *duart_cra = CR_ENABLE_TX;
    *duart_opr_set = 0x10;
}

static inline void disable_tx_a(void) {
    /* The Philips datasheet says when disabling tx, wait until TxEmp is set */
    *duart_opr_set = 0x08;
    while (!(*duart_sra & SR_TX_EMPTY)) {
        ;
    }
    *duart_opr_reset = 0x08;

    *duart_cra = CR_DISABLE_TX;
    *duart_opr_reset = 0x10;
}

ISR duart_isr_handler() {
#if WITH_INTS
    uint8_t i_status = *duart_isr;

#if RX_INTS
    /* Hopefully the RxRdy bit will be set */
    if (i_status & GLOBAL_ISR_RXRDY_A) {
        if (!cb_is_full(&rxbuff_a)) {
            cb_insert(&rxbuff_a, *duart_rba);

            rx++;
            if (rx & 1) { 
                *duart_opr_set = 0x80;
            } else {
                *duart_opr_reset = 0x80;
            }
        }
        else {
            /* discard the character */
            uint8_t ch = *duart_rba;

            /* nop */
            if (ch) {
                *duart_opr_set = 0x04;
            }
        }
    }
#endif

#if TX_INTS
    /* Has a character been transmitted? */
    if (i_status & GLOBAL_ISR_TXRDY_A) {
        *duart_opr_set = 0x20;

        /* Is there anything in the TX buffer? */
        if (cb_is_empty(&txbuff_a)) {
            /* The previous character was sent and the buffer is empty - disable TX */
            disable_tx_a();
        }
        else {
            *duart_tba = cb_remove(&txbuff_a);

            tx++;
            if (tx & 1) { 
                *duart_opr_set = 0x40;
            } else {
                *duart_opr_reset = 0x40;
            }
        }

    }
#endif

#endif /* WITH_INTS */
}

int _buffered_getchar(void) {
#if RX_INTS
    while (cb_is_empty(&rxbuff_a)) {
        ;
    }

    return cb_remove(&rxbuff_a);
#else
    return _polled_getchar();
#endif
}

int _buffered_char_available() {
#if RX_INTS
    return (rxbuff_a.remove != rxbuff_a.insert);
#else
    return _polled_char_available();
#endif
}

void _buffered_putchar(int ch) {
#if TXINTS
    /* Add it to the TX buffer */
    cb_insert(&txbuff_a, ch);
    enable_tx_a();
#else
    _polled_putchar(ch);
#endif
}

void _claim_duart(void) {
#if WITH_INTS
    uint8_t duart_vector_number = *duart_ivr;   /* Grab the vector number used by the duart */
#endif

    uint8_t imr = 0;

#if WITH_INTS
    /* Plug in the new interrupt handler */
    saved_duart_isr = set_isr_handler(duart_vector_number, (unsigned int)duart_isr_handler);
#endif

    *duart_cra = CR_RESET_MR_PTR;
    *duart_cra = CR_RESET_TX;

    /* Channel A */
    *duart_mr1a = 0x93;     /* RX: RTS control 8-1-N */
    *duart_mr2a = 0x07;     /* TX: Stop bit length=1 */

    /* This is apparently required to enable RTS to be managed by the duart. */
    *duart_opr_set = 0x03;  /* Assert RTS */

    *duart_acr  = 0x70;     /* Set 1 BRG */
    *duart_csra = 0x88;     /* 115200 (x2) */

#if RX_INTS
    rxbuff_a.insert = rxbuff_a.remove = 0;
#endif

#if TX_INTS
    txbuff_a.insert = txbuff_a.remove = 0;
#endif

    /* Interrupt on RxRdy and TxRdy */
#if TXINTS
    imr |= GLOBAL_ISR_TXRDY_A;
#endif

#if RX_INTS
    imr |= GLOBAL_ISR_RXRDY_A;
#endif

    *duart_imr = imr;

    *duart_cra = CR_ENABLE_RX;
    enable_tx_a();
}

void _release_duart(void) {
#if WITH_INTS
    uint8_t duart_vector_number = *duart_ivr;   /* Grab the vector number used by the duart */

#if TX_INTS
    /* Wait for TX buffer to empty */
    while (txbuff_a.remove != txbuff_a.insert) {
        ;
    }
#endif

    printf("\n--------------------\nChannel A:\n");
#if RX_INTS
    printf("RX insert =%d, RX remove=%d\n", rxbuff_a.insert, rxbuff_a.remove);
#endif

#if TX_INTS
    printf("TX insert=%d, TX remove=%d\n", txbuff_a.insert, txbuff_a.remove);
#endif

    /* Turn off all duart interrupts */
    *duart_imr = 0;
    set_isr_handler(duart_vector_number, saved_duart_isr);
#endif
}
