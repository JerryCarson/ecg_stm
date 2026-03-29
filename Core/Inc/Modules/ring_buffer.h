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

#endif
