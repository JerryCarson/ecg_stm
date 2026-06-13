#include "uplink_buffer.h"

Uplink_USB_Stream EXT_ADC1_Stream = {.queueHead = 0,
                                     .queueTail = 0};
Uplink_USB_Stream EXT_ADC2_Stream = {.queueHead = 0,
                                     .queueTail = 0};
Uplink_USB_Stream INT_ADC_Stream = {.queueHead = 0,
                                    .queueTail = 0};

static inline bool queueFull(Uplink_USB_Stream *stream)
{
    return ((stream->queueHead + 1U) & (MAX_QUEUE - 1U)) == stream->queueTail;
}

static inline bool queueEmpty(Uplink_USB_Stream *stream)
{
    const uint32_t head = (uint32_t)__atomic_load_n(&(stream->queueHead), __ATOMIC_RELAXED);
    const uint32_t tail = (uint32_t)__atomic_load_n(&(stream->queueTail), __ATOMIC_RELAXED);
    return head == tail; // TODO проверить работоспособность
}

StreamPacket_t *peekPacket(Uplink_USB_Stream *stream) //-V2506
{
    if (queueEmpty(stream))
    {
        return NULL;
    }

    return &(stream->packetQueue[stream->queueTail]);
}

void consumePacket(Uplink_USB_Stream *stream)
{
    // __disable_irq();
    if (!queueEmpty(stream))
    {
        stream->queueTail = (uint8_t)((stream->queueTail + 1U) & (MAX_QUEUE - 1U));
    }
    // __enable_irq();
}

void pushPacket(Uplink_USB_Stream *stream, StreamPacket_t *packet)
{
    bool full;
    uint8_t idx;
    __disable_irq();
    full = queueFull(stream);
    if (!full)
    {
        idx = stream->queueHead;
        stream->queueHead = (uint8_t)((stream->queueHead + 1U) & (MAX_QUEUE - 1U));
    }
    __enable_irq();

    if (!full)
    {
        StreamPacket_t *qPacket = &(stream->packetQueue[idx]);
        qPacket->dataType = packet->dataType;
        qPacket->length = (uint16_t)(packet->length > MAX_PACKET_SIZE ? MAX_PACKET_SIZE : packet->length);
        (void)memcpy(qPacket->data, packet->data, qPacket->length);
    }
}

void stream_data_uplink(Uplink_USB_Stream *stream) // TODO Возможно стоит перенести в ring_buffer
{
    StreamPacket_t *pkt = peekPacket(stream);
    __DMB();
    if (pkt != NULL)
    {
        if (pkt->length > MAX_PACKET_SIZE)
        {
            consumePacket(stream);
            return;
        }
        static uint8_t buf[MAX_PACKET_SIZE + HEADER_SIZE + CRC_SIZE];
        buf[0] = (uint8_t)CMD_HEADER;
        buf[1] = (uint8_t)(pkt->dataType);
        buf[2] = (uint8_t)((pkt->length) >> 8U);
        buf[3] = (uint8_t)((pkt->length) & 0xFFU);
        for (size_t i = 0U; i < pkt->length; i++)
        {
            buf[HEADER_SIZE + i] = pkt->data[i];
        }
        uint16_t total = (uint16_t)(HEADER_SIZE + pkt->length + CRC_SIZE - 1U);

        CRC->CR = (CRC->CR & ~CRC_CR_POLYSIZE_Msk) | CRC_CR_POLYSIZE_1;
        CRC->POL = 0x07U;  // Polynomial
        CRC->INIT = 0x00U; // Initial CRC value
        CRC->CR |= CRC_CR_RESET;
        // __DSB(); //TODO проверить работоспособность
        // __ISB();
        while ((CRC->CR & CRC_CR_RESET) != 0U)
        {
        };

        for (uint16_t i = 0U; i < total; i++)
        {
            *(__IO uint8_t *)&CRC->DR = buf[i];
        }

        uint8_t crc = (uint8_t)CRC->DR;

        buf[HEADER_SIZE + pkt->length] = crc;

        if (CDC_Transmit_FS(buf, (uint16_t)(pkt->length + HEADER_SIZE + CRC_SIZE)) == USBD_OK)
        {
            consumePacket(stream);
        }
    }
}