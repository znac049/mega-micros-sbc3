#include <stdio.h>
#include <machine.h>

static unsigned int saved_duart_isr;

static struct circular_buffer rxbuff_a;
static struct circular_buffer txbuff_a;

#if 0
static uint8_t tx=0;
#endif

static uint8_t rx=0;

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

ISR duart_isr_handler() {
    uint8_t i_status = *duart_isr;

    /* Hopefully the RxRdy bit will be set */
    if (i_status & GLOBAL_ISR_RXRDY_A_MASK) {
        if (!cb_is_full(&rxbuff_a)) {
            cb_insert(&rxbuff_a, *duart_rba);
            rxbuff_a.buffer[rxbuff_a.insert++] = *duart_rba;
            if (rxbuff_a.insert >= CIRCULAR_BUFFER_SIZE) {
                rxbuff_a.insert = 0;
            }

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

#if 0
    /* Has a character been transmitted? */
    if (i_status & GLOBAL_ISR_TXRDY_A_MASK) {
        /* Is there anything in the TX buffer? */
        if (cb_is_empty(&txbuff_a)) {
            /* The previous character was sent and the buffer is empty - disable TX */
            *duart_cra = CR_DISABLE_TX;
            *duart_opr_reset = 0x20;
        }
        else {
            *duart_tba = cb_remove(&txbuff_a);
        }

        tx++;
        if (tx & 1) { 
            *duart_opr_set = 0x40;
        } else {
            *duart_opr_reset = 0x40;
        }
    }
#endif
}

void set_led(int lednum) {
    if ((lednum < 5) || (lednum > 10)) {
        return;
    }

    *duart_opr_set = 1<<(lednum - 2);
}

void clear_led(int lednum) {
    if ((lednum < 5) || (lednum > 10)) {
        return;
    }

    *duart_opr_reset = 1<<(lednum - 2);
}

int _getchar(void) {
    while (cb_is_empty(&rxbuff_a)) {
        ;
    }

    return cb_remove(&rxbuff_a);
}

int _char_available() {
    return (rxbuff_a.remove != rxbuff_a.insert);
}

void _putchar_buffered(int ch) {
    uint8_t was_empty = cb_is_empty(&txbuff_a);
    
    /* Add it to the TX buffer */
    cb_insert(&txbuff_a, ch);
    if (was_empty) {
        /* Enable TX, which should generate an immediate TXRdy interrupt */
        *duart_cra = CR_ENABLE_TX;
        *duart_opr_set = 0x20;
        while (!(*duart_sra & SR_TXRDY_MASK)) {
            ;
        }
     }
}

void init_duart(void) {
    uint8_t duart_vector_number = *duart_ivr;   /* Grab the vector number used by the duart */

    /* Channel A */
    *duart_mr1a = 0x93;     /* RX: RTS control 8-1-N */
    *duart_mr2a = 0x07;     /* TX: Stop bit length=1 */
#if 0
    *duart_cra = CR_DISABLE_TX;
#endif

    /* This is apparently required to enable RTS to be managed by the duart. */
    *duart_opr_set = 0x03;  /* Assert RTS */

    *duart_acr  = 0x70;     /* Set 1 BRG */
    *duart_csra = 0x88;     /* 115200 (x2) */
    *duart_csrb = 0x88;     /* 115200 (x2) */

    rxbuff_a.insert = rxbuff_a.remove = 0;

    /* So we can restore it on exit */
    saved_duart_isr = get_isr_handler(duart_vector_number);

    /* Plug in the new interrupt handler */
    INTSOFF();
    set_isr_handler(duart_vector_number, (unsigned int)duart_isr_handler);

    /* Interrupt on RxRdy and TxRdy */
    *duart_imr = /* GLOBAL_ISR_TXRDY_A_MASK | */ GLOBAL_ISR_RXRDY_A_MASK;

    /* Unleash the hounds... */
    INTSON();
}

void close_duart(void) {
    uint8_t duart_vector_number = *duart_ivr;   /* Grab the vector number used by the duart */

    /* Wait for TX buffer to empty */
    while (txbuff_a.remove != txbuff_a.insert) {
        ;
    }

    printf("\n--------------------\nChannel A:\n");
    printf("RX insert =%d, RX remove=%d\n", rxbuff_a.insert, rxbuff_a.remove);
    printf("TX insert=%d, TX remove=%d\n", txbuff_a.insert, txbuff_a.remove);

    /* Turn off all duart interrupts */
    *duart_imr = 0;
    set_isr_handler(duart_vector_number, saved_duart_isr);
}
