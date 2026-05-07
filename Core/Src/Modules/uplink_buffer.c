#include "uplink_buffer.h"

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

void stream_data_uplink(Uplink_USB_Stream *stream) // TODO Возможно стоит перенести в ring_buffer
{
    StreamPacket_t *pkt = peekPacket(stream);
    __DMB();
    if (pkt)
    {
        if (pkt->length > MAX_PACKET_SIZE)
        {
            consumePacket(stream);
            return;
        }
        static uint8_t buf[MAX_PACKET_SIZE + HEADER_SIZE + CRC_SIZE];
        buf[0] = CMD_HEADER;
        buf[1] = pkt->dataType;
        buf[2] = (pkt->length) >> 8;
        buf[3] = (pkt->length) & 0xFF;
        for (size_t i = 0; i < pkt->length; i++)
        {
            buf[HEADER_SIZE + i] = pkt->data[i];
        }
        uint16_t total = HEADER_SIZE + pkt->length + CRC_SIZE - 1;

        CRC->CR = (CRC->CR & ~CRC_CR_POLYSIZE_Msk) | CRC_CR_POLYSIZE_1;
        CRC->POL = 0x07;  // Polynomial
        CRC->INIT = 0x00; // Initial CRC value
        CRC->CR |= CRC_CR_RESET;
        __DSB();
        __ISB();
        while (CRC->CR & CRC_CR_RESET)
            ;

        for (uint16_t i = 0; i < total; i++)
        {
            *(__IO uint8_t *)&CRC->DR = buf[i];
        }

        uint8_t crc = (uint8_t)CRC->DR;

        buf[HEADER_SIZE + pkt->length] = crc;

        if (CDC_Transmit_FS(buf, pkt->length + HEADER_SIZE + CRC_SIZE) == USBD_OK)
        {
            consumePacket(stream);
        }
    }
}