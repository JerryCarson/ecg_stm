/**
 * @file usb_parser.h
 * @brief Интерфейс парсера входящих/исходящих потоков USB CDC.
 * @details Реализует буферизацию, фрейминг пакетов, валидацию CRC-8
 *          и диспетчеризацию команд. Оптимизирован для однопоточного
 *          доступа с атомарным обновлением индексов.
 * @addtogroup USB_PARSER Интерфейс парсера входящих/исходящих потоков USB CDC
 * @{
 */

#ifndef __USB_PARSER_H__
#define __USB_PARSER_H__

#include "main.h"
#include "string.h"
#include "ring_buffer.h"

#define PARSER_BUFFER_SIZE 512U                       /**< Размер кольцевого буфера входящего потока [байт]. @note Должен быть степенью двойки для маскирования индексов. */
#define MAX_PAYLOAD 256U                              /**< Максимальная полезная нагрузка пакета [байт]. */
#define HEADER_SIZE 4U                                /**< Размер заголовка пакета: [SYNC][TYPE][LEN_H][LEN_L] [байт]. */
#define CRC_SIZE 1U                                   /**< Размер контрольной суммы CRC-8 [байт]. */
#define MIN_PACKET_SIZE (HEADER_SIZE + CRC_SIZE + 1U) /**< Минимальный валидный размер пакета [байт]. */

_Static_assert((PARSER_BUFFER_SIZE & (PARSER_BUFFER_SIZE - 1)) == 0,
               "PARSER_BUFFER_SIZE must be power of two");

_Static_assert((PARSER_BUFFER_SIZE > (MAX_PAYLOAD + HEADER_SIZE + CRC_SIZE)),
               "Too big payload");

/**
 * @brief Структура кольцевого буфера для входящих данных (PC → MCU).
 * @details Предназначен для временного хранения сырых байтов, принимаемых
 *          по USB CDC.
 */
typedef struct Downlink_USB_Stream
{
    uint8_t buffer[PARSER_BUFFER_SIZE]; /**< Массив хранения данных */
    volatile uint16_t head;             /**< Индекс записи (следующая свободная ячейка) */
    volatile uint16_t tail;             /**< Индекс чтения (следующий байт для парсинга) */
} Downlink_USB_Stream;

/**
 * @brief Парсинг и валидация входящего потока от ПК.
 * @details Извлекает полные пакеты из буфера, проверяет заголовок,
 *          длину и CRC-8. Валидные команды передаются в @ref process_command().
 *          При ошибке синхронизации или CRC буфер продвигается до следующего
 *          валидного маркера @ref CMD_HEADER.
 * @param[in] s Указатель на структуру входящего потока. Не должен быть @c NULL.
 */
void parse_data_downlink(Downlink_USB_Stream *s);

/**
 * @brief Формирование и отправка пакетов в исходящий поток (MCU → PC).
 * @details Извлекает пакет из очереди, формирует кадр с заголовком и CRC-8,
 *          вычисляет контрольную сумму аппаратным блоком CRC и передаёт
 *          через @ref CDC_Transmit_FS(). При @c USBD_OK пакет помечается как обработанный.
 * @param[in] stream Указатель на структуру исходящего потока. Не должен быть @c NULL.
 * @note Сбрасывает конфигурацию CRC при каждом вызове. Не гарантирует
 *       успешную отправку, если USB-стек занят (требует внешней проверки статуса).
 */
void stream_data_uplink(Uplink_USB_Stream *stream);

/**
 * @brief Безопасная запись данных в кольцевой буфер входящего потока.
 * @details Рассчитана на однопоточный вызов со стороны приёмника @ref CDC_Receive_FS. Автоматически обрабатывает переполнение, 
 *          не перезаписывая непрочитанные данные.
 * @param[in] s Указатель на структуру буфера. Не должен быть @c NULL.
 * @param[in] data Указатель на источник данных для записи.
 * @param[in] len Количество байт для записи.
 * @return Фактическое количество записанных байт. Может быть меньше @c len 
 *         при недостатке свободного места.
 * @note Использует `__DMB()`/`__DSB()` для обеспечения порядка операций 
 *       с индексами.
 */
uint16_t stream_write(Downlink_USB_Stream *s, const uint8_t *data, uint16_t len);

extern Downlink_USB_Stream usbStream;
/** @} */
#endif