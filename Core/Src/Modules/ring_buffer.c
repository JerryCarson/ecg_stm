#include "ring_buffer.h"

Uplink_USB_Stream EXT_ADC1_Stream = {.queueHead = 0,
                                .queueTail = 0};
Uplink_USB_Stream EXT_ADC2_Stream = {.queueHead = 0,
                                .queueTail = 0};
Uplink_USB_Stream INT_ADC_Stream = {.queueHead = 0,
                               .queueTail = 0};

static inline int queueFull(Uplink_USB_Stream *stream)
{
    return ((stream->queueHead + 1) & (MAX_QUEUE - 1)) == stream->queueTail;
}

static inline int queueEmpty(Uplink_USB_Stream *stream)
{
    return stream->queueHead == stream->queueTail;
}

StreamPacket_t *peekPacket(Uplink_USB_Stream *stream)
{
    if (queueEmpty(stream))
        return NULL;

    return &(stream->packetQueue[stream->queueTail]);
}

void consumePacket(Uplink_USB_Stream *stream)
{
    __disable_irq();
    if (!queueEmpty(stream))
        stream->queueTail = (stream->queueTail + 1) & (MAX_QUEUE - 1);
    __enable_irq();
}

// void pushPacket(StreamPacket_t *packet)
// {
//     int full;
//     uint8_t idx;
//     __disable_irq();
//     full = queueFull();
//     if (!full)
//     {
//         idx = queueHead;
//         queueHead = (queueHead + 1) & (MAX_QUEUE - 1);
//     }
//     __enable_irq();

//     if (!full)
//     {
//         StreamPacket_t *qPacket = &packetQueue[idx];
//         qPacket->dataType = packet->dataType;
//         qPacket->length = packet->length > MAX_PACKET_SIZE ? MAX_PACKET_SIZE : packet->length;
//         memcpy(qPacket->data, packet->data, qPacket->length);
//     }
// }

void pushPacket(Uplink_USB_Stream *stream, StreamPacket_t *packet)
{
    int full;
    uint8_t idx;
    __disable_irq();
    full = queueFull(stream);
    if (!full)
    {
        idx = stream->queueHead;
        stream->queueHead = (stream->queueHead + 1) & (MAX_QUEUE - 1);
    }
    __enable_irq();

    if (!full)
    {
        StreamPacket_t *qPacket = &(stream->packetQueue[idx]);
        qPacket->dataType = packet->dataType;
        qPacket->length = packet->length > MAX_PACKET_SIZE ? MAX_PACKET_SIZE : packet->length;
        memcpy(qPacket->data, packet->data, qPacket->length);
    }
}