#include "ring_buffer.h"

#define MAX_QUEUE 16
_Static_assert((MAX_QUEUE & (MAX_QUEUE - 1)) == 0,
               "MAX_QUEUE must be power of two");

StreamPacket_t packetQueue[MAX_QUEUE];
volatile uint8_t queueHead = 0;
volatile uint8_t queueTail = 0;

static inline int queueFull()
{
    return ((queueHead + 1) & (MAX_QUEUE - 1)) == queueTail;
}

static inline int queueEmpty()
{
    return queueHead == queueTail;
}

void pushPacket(StreamPacket_t *packet)
{
    int full;
    uint8_t idx;
    __disable_irq();
    full = queueFull();
    if (!full)
    {
        idx = queueHead;
        queueHead = (queueHead + 1) & (MAX_QUEUE - 1);
    }
    __enable_irq();

    if (!full)
    {
        StreamPacket_t *qPacket = &packetQueue[idx];
        qPacket->dataType = packet->dataType;
        qPacket->length = packet->length > MAX_PACKET_SIZE ? MAX_PACKET_SIZE : packet->length;
        memcpy(qPacket->data, packet->data, qPacket->length);
    }
}
