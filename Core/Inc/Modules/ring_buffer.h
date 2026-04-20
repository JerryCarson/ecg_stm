#ifndef __RING_BUFFER_H__
#define __RING_BUFFER_H__

#include "main.h"

#define MAX_QUEUE 32
_Static_assert((MAX_QUEUE & (MAX_QUEUE - 1)) == 0,
               "MAX_QUEUE must be power of two");

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
typedef struct StreamPacket_t
{
    StreamDataType dataType;       /**< Тип данных в пакете */
    uint16_t length;               /**< Количество байтов данных в пакете */
    uint8_t data[MAX_PACKET_SIZE]; /**< Массив байтов данных */
} StreamPacket_t;

typedef struct Uplink_USB_Stream
{
    StreamPacket_t packetQueue[MAX_QUEUE];
    volatile uint8_t queueHead;
    volatile uint8_t queueTail;
} Uplink_USB_Stream;

/** @brief Добавляет в кольцевой буфер полученный пакет данных */
void pushPacket(Uplink_USB_Stream *stream, StreamPacket_t *packet);
StreamPacket_t *peekPacket(Uplink_USB_Stream *stream);
void consumePacket(Uplink_USB_Stream *stream);

extern Uplink_USB_Stream EXT_ADC1_Stream;
extern Uplink_USB_Stream EXT_ADC2_Stream;
extern Uplink_USB_Stream INT_ADC_Stream;

#endif
