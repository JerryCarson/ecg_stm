#ifndef __USB_PARSER_H__
#define __USB_PARSER_H__

#include "main.h"
#include "string.h"

// typedef enum
// {
//     STATE_WAIT_HEADER,
//     STATE_WAIT_DATA_TYPE,
//     STATE_WAIT_LEN0,
//     STATE_WAIT_LEN1,
//     STATE_WAIT_DATA,
//     STATE_WAIT_CRC
// } USBData_ParseState;

// extern uint8_t flag;
// extern uint32_t expected_len;
// extern uint8_t tm_buf[8];
// extern uint8_t payload[MAX_PAYLOAD];

// uint8_t crc8_calc(uint8_t *data, uint32_t len);

// void parse_usb_data(uint8_t byte);

/*================================================================*/

/** @brief  Определение типа указателя на функцию, используется для создания таблицы команд.*/
typedef void (*cmd_handler_t)(uint8_t *payload, uint16_t len);

#define PARSER_BUFFER_SIZE 512 /**< Размер кольцевого буфера с которым работает парсер команд от ПК (в байтах). */
#define MAX_PAYLOAD 256        /**< Максимальный допустимый размер данных в пакете (в байтах). */

_Static_assert((PARSER_BUFFER_SIZE & (PARSER_BUFFER_SIZE - 1)) == 0,
               "PARSER_BUFFER_SIZE must be power of two");

#define HEADER_SIZE 4                                /**< Пакет начинается с заголовочного байта, байта типа данных и двух байтов длины последующих данных. */
#define CRC_SIZE 1                                   /**< В конце пакета содержится 1 байт контрольной суммы. */
#define MIN_PACKET_SIZE (HEADER_SIZE + CRC_SIZE + 1) /**< Минимальный пакет данных должен содержать хотя бы 1 байт данных. */

_Static_assert((PARSER_BUFFER_SIZE > (MAX_PAYLOAD + HEADER_SIZE + CRC_SIZE)),
               "Too big payload");


/** @brief Структура, определяющая кольцевой буфер \ref usbStream, обрабатываемый парсером команд от ПК. */
typedef struct
{
    uint8_t buffer[PARSER_BUFFER_SIZE];

    volatile uint16_t head;
    volatile uint16_t tail;

} USBStream;

/** 
 * @brief Функция обнаружения и обработки пакета в получаемых от ПК данных.
 * 
 * @param s Указатель на кольцевой буфер \ref usbStream, с которым работает парсер.
 * */
void parser_process(USBStream *s);

/**
 * @brief Функция записи данных в кольцевой буфер \ref usbStream.
 * 
 * @param s Указатель на кольцевой буфер \ref usbStream, с которым работает парсер.
 * @param data Указатель на массив данных, которые требуется записать в буфер.
 * @param len Длина записываемых данных.
 */
uint16_t stream_write(USBStream *s, const uint8_t *data, uint16_t len);

extern USBStream usbStream;

#endif