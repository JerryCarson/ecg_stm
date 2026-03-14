#include "ring_buffer.h"

#define MAX_QUEUE 8
StreamPacket_t packetQueue[MAX_QUEUE];
volatile uint8_t queueHead = 0;
volatile uint8_t queueTail = 0;

static inline int queueFull()
{
    return ((queueHead + 1) % MAX_QUEUE) == queueTail;
}

static inline int queueEmpty()
{
    return queueHead == queueTail;
}

void pushPacket(StreamPacket_t *packet)
{
    if (!queueFull())
    {
        packetQueue[queueHead] = *packet; // shallow copy, data must persist
        queueHead = (queueHead + 1) % MAX_QUEUE;
    }
    else
    {
        // Queue full, drop packet or set an error flag
    }
}
