#include <machine.h>

void cb_reset(circular_buffer_t *cb) {
    cb->insert = cb->remove = 0;
    cb->free = CIRCULAR_BUFFER_SIZE;
}   

bool_t cb_is_empty(struct circular_buffer *cb) {
    return cb->insert == cb->remove;
}

bool_t cb_is_full(struct circular_buffer *cb) {
    uint16_t ins = cb->insert+1;

    if (ins >= CIRCULAR_BUFFER_SIZE)
        ins = 0;

    return ins == cb->remove;
}

void cb_insert(struct circular_buffer *cb, uint8_t ch) {
    cb->buffer[cb->insert++] = ch;

    if (cb->insert >= CIRCULAR_BUFFER_SIZE)
        cb->insert = 0;

    cb->free--;
}

int cb_remove(struct circular_buffer *cb) {
    int res;

    if (cb_is_empty(cb)) 
        return -1;

    res = cb->buffer[cb->remove++];

    if (cb->remove >= CIRCULAR_BUFFER_SIZE)
        cb->remove = 0;

    cb->free++;
    
    return res;
}
