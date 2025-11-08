#include "circular_buffer.h"

void circular_buffer_init(circular_buffer_t *cb, uint8_t *buffer, size_t size) {
    cb->buffer = buffer;
    cb->size = size;
    cb->head = 0;
    cb->tail = 0;
    cb->count = 0;
}

bool circular_buffer_push(circular_buffer_t *cb, uint8_t data) {
    if (cb->count >= cb->size) {
        // Drop oldest
        cb->tail = (cb->tail + 1) % cb->size;
        cb->count--;
    }
    cb->buffer[cb->head] = data;
    cb->head = (cb->head + 1) % cb->size;
    cb->count++;
    return true;
}

bool circular_buffer_pop(circular_buffer_t *cb, uint8_t *data) {
    if (cb->count == 0) {
        return false;
    }
    *data = cb->buffer[cb->tail];
    cb->tail = (cb->tail + 1) % cb->size;
    cb->count--;
    return true;
}

size_t circular_buffer_available(circular_buffer_t *cb) {
    return cb->count;
}

size_t circular_buffer_free(circular_buffer_t *cb) {
    return cb->size - cb->count;
}

void circular_buffer_clear(circular_buffer_t *cb) {
    cb->head = 0;
    cb->tail = 0;
    cb->count = 0;
}
