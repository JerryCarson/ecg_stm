/**
 * @file downlink_parser.h
 * @brief Интерфейс парсера входящих/исходящих потоков USB CDC.
 * @details Реализует буферизацию, фрейминг пакетов, валидацию CRC-8
 *          и диспетчеризацию команд. Оптимизирован для однопоточного
 *          доступа с атомарным обновлением индексов.
 * @addtogroup DOWNLINK_PARSER Интерфейс парсера входящих/исходящих потоков USB CDC
 * @{
 */

#ifndef DOWNLINK_PARSER_H
#define DOWNLINK_PARSER_H

#include "main.h"
#include "string.h"
// #include "uplink_buffer.h"

#define PARSER_BUFFER_SIZE 512U /**< Размер кольцевого буфера входящего потока [байт]. @note Должен быть степенью двойки для маскирования индексов. */
#define MAX_PAYLOAD 256U        /**< Максимальная полезная нагрузка пакета [байт]. */

_Static_assert((PARSER_BUFFER_SIZE & (PARSER_BUFFER_SIZE - 1U)) == 0U,
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
    volatile uint32_t head;             /**< Индекс записи (следующая свободная ячейка) */
    volatile uint32_t tail;             /**< Индекс чтения (следующий байт для парсинга) */
} Downlink_USB_Stream;

/**
 * @brief Парсинг и валидация входящего потока от ПК.
 * @details Извлекает полные пакеты из буфера, проверяет заголовок,
 *          длину и CRC-8. Валидные команды передаются в @ref process_command().
 *          При ошибке синхронизации или CRC буфер продвигается до следующего
 *          валидного маркера @ref CMD_HEADER.
 * @param[in] s Указатель на структуру входящего потока. Не должен быть @c NULL.
 */
void parse_downlink_data(Downlink_USB_Stream *s);

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
static inline uint32_t stream_write(Downlink_USB_Stream *s, const uint8_t *data, uint32_t len)
{
    // ⚠️ Cache volatile reads ONCE at start
    // uint16_t head = s->head;
    // uint16_t tail = s->tail;
    // TODO проверить работоспособность
    const uint32_t head = (uint32_t)__atomic_load_n(&(s->head), __ATOMIC_ACQUIRE);
    const uint32_t tail = (uint32_t)__atomic_load_n(&(s->tail), __ATOMIC_RELAXED);

    // Calculate free space using cached values only
    uint32_t free;
    if (head >= tail)
    {
        free = PARSER_BUFFER_SIZE - (head - tail) - 1U;
    }
    else
    {
        free = tail - head - 1U;
    }

    if (len > free)
    {
        len = free;
    }

    if (len == 0U)
    {
        return 0U;
    } // Buffer full

    // Write data to buffer (wrap-around safe)
    uint32_t first = PARSER_BUFFER_SIZE - head;
    if (first > len)
    {
        first = len;
    }

    (void)memcpy(&s->buffer[head], data, first);
    (void)memcpy(&s->buffer[0], data + first, len - first);

    // __DMB();

    // Update head (this is the ONLY writer of head)
    // s->head = (head + len) & (PARSER_BUFFER_SIZE - 1);
    // TODO проверить работоспособность
    __atomic_store_n(&(s->head), ((head + len) & (PARSER_BUFFER_SIZE - 1U)), __ATOMIC_RELEASE); //-V2547 not a function

    return len;
}

extern Downlink_USB_Stream usbStream;
/** @} */
#endif