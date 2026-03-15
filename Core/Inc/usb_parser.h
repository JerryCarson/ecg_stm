#ifndef __USB_PARSER_H__
#define __USB_PARSER_H__

#include "main.h"
#include "string.h"

typedef enum
{
    STATE_WAIT_HEADER,
    STATE_WAIT_DATA_TYPE,
    STATE_WAIT_LEN0,
    STATE_WAIT_LEN1,
    STATE_WAIT_DATA,
    STATE_WAIT_CRC
} USBData_ParseState;

extern uint8_t flag;
extern uint32_t expected_len;
extern uint8_t tm_buf[8];
// extern uint8_t payload[MAX_PAYLOAD];

// uint8_t crc8_calc(uint8_t *data, uint32_t len);

// void parse_usb_data(uint8_t byte);

/*================================================================*/

typedef void (*cmd_handler_t)(uint8_t *payload, uint16_t len);

#define PARSER_BUFFER_SIZE 512
#define MAX_PAYLOAD 256

_Static_assert((PARSER_BUFFER_SIZE & (PARSER_BUFFER_SIZE - 1)) == 0,
               "PARSER_BUFFER_SIZE must be power of two");

#define HEADER_SIZE 4
#define CRC_SIZE 1
#define MIN_PACKET_SIZE (HEADER_SIZE + CRC_SIZE + 1)

_Static_assert((PARSER_BUFFER_SIZE > (MAX_PAYLOAD + HEADER_SIZE + CRC_SIZE)),
               "Too big payload");

typedef struct
{
    uint8_t buffer[PARSER_BUFFER_SIZE];

    volatile uint16_t head;
    volatile uint16_t tail;

} USBStream;

void parser_process(USBStream *s);
uint16_t stream_write(USBStream *s, const uint8_t *data, uint16_t len);

extern USBStream usbStream;

#endif