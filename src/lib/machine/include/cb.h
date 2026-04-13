#pragma once

#include <ctype.h>
#define CIRCULAR_BUFFER_SIZE 128

struct circular_buffer {
    uint8_t buffer[CIRCULAR_BUFFER_SIZE];
    volatile uint16_t insert;
    volatile uint16_t remove;
};

typedef struct circular_buffer circular_buffer_t;

int cb_remove(struct circular_buffer *cb);
void cb_insert(struct circular_buffer *cb, uint8_t ch);
bool_t cb_is_full(struct circular_buffer *cb);
bool_t cb_is_empty(struct circular_buffer *cb);
void cb_reset(circular_buffer_t *cb);