#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct {
    uint8_t *buffer;
    size_t size;
    size_t head;
    size_t tail;
    size_t count;
} circular_buffer_t;

void circular_buffer_init(circular_buffer_t *cb, uint8_t *buffer, size_t size);
bool circular_buffer_push(circular_buffer_t *cb, uint8_t data);
bool circular_buffer_pop(circular_buffer_t *cb, uint8_t *data);
size_t circular_buffer_available(circular_buffer_t *cb);
size_t circular_buffer_free(circular_buffer_t *cb);
void circular_buffer_clear(circular_buffer_t *cb);

#endif
