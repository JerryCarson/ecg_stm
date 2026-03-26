#ifndef __RING_BUFFER_H__
#define __RING_BUFFER_H__

#include "main.h"

/**
 * @brief Структура пакета данных для отправки с контроллера на ПК
 *
 * Содержит информацию о типе пакета (данные с внешних АЦП, с внутреннего АЦП),
 * длину полезной нагрузки и саму полезную нагрузку.
 * Типы пакетов можно добавлять в код, редактируя \ref StreamDataType.
 * По мере получения данных от периферии, кольцевой буфер, содержащий такие структуры,
 * заполняется полученными данными. Затем элементы буфера по очереди считываются и
 * отправляются на ПК.
 */
typedef struct
{
    StreamDataType dataType;       /**< Тип данных в пакете */
    uint16_t length;               /**< Количество байтов данных в пакете */
    uint8_t data[MAX_PACKET_SIZE]; /**< Массив байтов данных */
} StreamPacket_t;

/** @brief Добавляет в кольцевой буфер полученный пакет данных */
void pushPacket(StreamPacket_t *packet);
StreamPacket_t *peekPacket(void);
void consumePacket(void);
// TODO add this to sending logic
//  #include "usbd_cdc_if.h" // for CDC_Transmit_FS

// int sendPacketCDC(const StreamPacket_t *packet)
// {
//     // Packet size: header(1) + type(1) + len(2) + data + crc(1)
//     uint16_t txLen = 1 + 1 + 2 + packet->length + 1;

//     // Make sure it fits USB max packet (usually 64 bytes for full-speed)
//     if (txLen > 256)  // adjust if you want larger buffers
//         return -1;     // too big

//     uint8_t txBuf[256];  // local buffer

//     uint16_t idx = 0;
//     txBuf[idx++] = 0xAA;                // header
//     txBuf[idx++] = packet->dataType;    // data type
//     txBuf[idx++] = (packet->length >> 8) & 0xFF; // MSB
//     txBuf[idx++] = packet->length & 0xFF;        // LSB

//     // Copy data bytes
//     for (uint16_t i = 0; i < packet->length; i++)
//         txBuf[idx++] = packet->data[i];

//     // CRC8 over [data type, length MSB, length LSB, data bytes]
//     txBuf[idx++] = crc8(&txBuf[1], 1 + 2 + packet->length);

//     // Transmit via USB CDC
//     if (CDC_Transmit_FS(txBuf, idx) != USBD_OK)
//         return -2; // transmission failed

//     return 0; // success
// }

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
