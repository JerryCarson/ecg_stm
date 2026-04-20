#ifndef __USB_PARSER_H__
#define __USB_PARSER_H__

#include "main.h"
#include "string.h"
#include "ring_buffer.h"

#define PARSER_BUFFER_SIZE 512 /**< Размер кольцевого буфера с которым работает парсер команд от ПК (в байтах). */
#define MAX_PAYLOAD 256        /**< Максимальный допустимый размер данных в пакете (в байтах). */

_Static_assert((PARSER_BUFFER_SIZE & (PARSER_BUFFER_SIZE - 1)) == 0,
               "PARSER_BUFFER_SIZE must be power of two");

#define HEADER_SIZE 4                                /**< Пакет начинается с заголовочного байта, байта типа данных и двух байтов длины последующих данных. */
#define CRC_SIZE 1                                   /**< В конце пакета содержится 1 байт контрольной суммы. */
#define MIN_PACKET_SIZE (HEADER_SIZE + CRC_SIZE + 1) /**< Минимальный пакет данных должен содержать хотя бы 1 байт данных. */

_Static_assert((PARSER_BUFFER_SIZE > (MAX_PAYLOAD + HEADER_SIZE + CRC_SIZE)),
               "Too big payload");


/** @brief Структура, определяющая кольцевой буфер \ref Downlink_USB_Stream, обрабатываемый парсером команд от ПК. */
typedef struct Downlink_USB_Stream
{
    uint8_t buffer[PARSER_BUFFER_SIZE];

    volatile uint16_t head;
    volatile uint16_t tail;
} Downlink_USB_Stream;

/** 
 * @brief Функция обнаружения и обработки пакета в получаемых от ПК данных.
 * 
 * @param s Указатель на кольцевой буфер \ref Downlink_USB_Stream, с которым работает парсер.
 * */
void parse_data_downlink(Downlink_USB_Stream *s);


void stream_data_uplink(Uplink_USB_Stream *stream);

/**
 * @brief Функция записи данных в кольцевой буфер \ref Downlink_USB_Stream.
 * 
 * @param s Указатель на кольцевой буфер \ref Downlink_USB_Stream, с которым работает парсер.
 * @param data Указатель на массив данных, которые требуется записать в буфер.
 * @param len Длина записываемых данных.
 */
uint16_t stream_write(Downlink_USB_Stream *s, const uint8_t *data, uint16_t len);

extern Downlink_USB_Stream usbStream;

#endif