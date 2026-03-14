#ifndef __RING_BUFFER_H__
#define __RING_BUFFER_H__

#include "main.h"

typedef enum
{
    DATA_SPI_0,
    DATA_SPI_1,
    DATA_ADC_ECG,
    DATA_PACKET_ERROR
} StreamDataType;

typedef struct
{
    StreamDataType dataType;
    uint16_t length; // in bytes
    uint8_t *data;   // pointer to buffer
} StreamPacket_t;

void pushPacket(StreamPacket_t *packet);

/*
#define RINGBUFFER_DEFINE(type, name, size)                \
    typedef struct                                         \
    {                                                      \
        type buffer[size];                                 \
        volatile uint16_t head;                            \
        volatile uint16_t tail;                            \
    } name;                                                \
                                                           \
    static inline uint8_t name##_Put(name *rb, type data)  \
    {                                                      \
        uint16_t next = (rb->head + 1) & (size - 1);       \
                                                           \
        if (next == rb->tail)                              \
            return 0;                                      \
                                                           \
        rb->buffer[rb->head] = data;                       \
        rb->head = next;                                   \
                                                           \
        return 1;                                          \
    }                                                      \
                                                           \
    static inline uint8_t name##_Get(name *rb, type *data) \
    {                                                      \
        if (rb->tail == rb->head)                          \
            return 0;                                      \
                                                           \
        *data = rb->buffer[rb->tail];                      \
        rb->tail = (rb->tail + 1) & (size - 1);            \
                                                           \
        return 1;                                          \
    }
*/

#endif
