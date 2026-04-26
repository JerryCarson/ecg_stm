/**
 * @file ring_buffer.h
 * @brief Структура кольцевого буфера для передачи данных на ПК через USB CDC
 * @addtogroup RING_BUFFER Структура кольцевого буфера для передачи данных на ПК через USB CDC
 * @{
 */

#ifndef __RING_BUFFER_H__
#define __RING_BUFFER_H__

#include "main.h"

/**
 * @def MAX_QUEUE
 * @brief Максимальное количество пакетов в очереди. Должно быть степенью двойки.
 */
#define MAX_QUEUE 32

_Static_assert((MAX_QUEUE & (MAX_QUEUE - 1)) == 0,
               "MAX_QUEUE must be power of two");

/**
 * @brief Структура пакета данных для отправки на ПК.
 * @details Содержит метаданные (тип данных, длина полезной нагрузки) и саму
 *          полезную нагрузку. Типы данных определяются перечислением
 *          @ref StreamDataType. Максимальный размер полезной нагрузки задаётся
 *          макросом @ref MAX_PACKET_SIZE.
 * @note Все поля структуры должны быть корректно инициализированы перед
 *       передачей в @ref pushPacket.
 */
typedef struct StreamPacket_t
{
    StreamDataType dataType;       /**< Тип данных в пакете */
    uint16_t length;               /**< Длина полезной нагрузки в байтах. */
    uint8_t data[MAX_PACKET_SIZE]; /**< Массив полезной нагрузки. */
} StreamPacket_t;

/** @brief Структура очереди пакетов для передачи по USB. */
typedef struct Uplink_USB_Stream
{
    StreamPacket_t packetQueue[MAX_QUEUE]; /**< Массив пакетов данных */
    volatile uint8_t queueHead;            /**< Индекс записи (следующая свободная ячейка) */
    volatile uint8_t queueTail;            /**< Индекс чтения (следующий байт для отправки) */
} Uplink_USB_Stream;

/**
 * @brief Добавляет пакет в кольцевой буфер.
 * @param[in] stream Указатель на очередь пакетов.
 * @param[in] packet Указатель на добавляемый пакет.
 * @note Функция атомарна относительно прерываний (использует критическую секцию).
 *       Вызывается как из контекста прерывания так и из контекста основного цикла.
 *       Реентрантность соблюдена. //TODO Перепроверить
 * @warning Если очередь полна, пакет будет молча отброшен. Вызывающий код должен
 *          контролировать заполненность буфера или обрабатывать потерю данных.
 */
void pushPacket(Uplink_USB_Stream *stream, StreamPacket_t *packet); 

/**
 * @brief Возвращает указатель на пакет в хвосте очереди без изменения индексов.
 * @param[in] stream Указатель на очередь пакетов.
 * @return Указатель на следующий пакет для отправки.
 * @retval NULL Если очередь пуста.
 */
StreamPacket_t *peekPacket(Uplink_USB_Stream *stream);

/**
 * @brief Удаляет пакет из хвоста очереди (продвигает индекс чтения).
 * @param[in] stream Указатель на очередь пакетов.
 * @note Функция атомарна относительно прерываний. Должна вызываться только
 *       после успешной отправки пакета, полученного через @ref peekPacket
 *       или для удаления из очереди слишком длинного пакета.
 */
void consumePacket(Uplink_USB_Stream *stream);

/** @brief Очередь пакетов для I ADC */
extern Uplink_USB_Stream EXT_ADC1_Stream;

/** @brief Очередь пакетов для II ADC */
extern Uplink_USB_Stream EXT_ADC2_Stream;

/** @brief Очередь пакетов для сигнала ECG */
extern Uplink_USB_Stream INT_ADC_Stream;

/** @} */
#endif
