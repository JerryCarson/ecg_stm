#include "usb_parser.h"

// #define OTG_BUFFER_SIZE 64 // TODO refactor

// USBData_ParseState parse_state = STATE_WAIT_HEADER;
// static uint8_t current_cmd;
// static uint16_t expected_len0;
// static uint16_t expected_len1;
// uint32_t expected_len;
// static uint32_t data_index;
// static uint8_t parse_buffer[OTG_BUFFER_SIZE];
// uint8_t pbi = 0;
// uint8_t head_size;

// uint8_t crc8_calc(uint8_t *data, uint32_t len)
// {
//     uint8_t crc = 0x00;
//     for (uint32_t i = 0; i < len; i++)
//     {
//         crc ^= data[i];
//         for (uint32_t j = 0; j < 8; j++)
//         {
//             if (crc & 0x80)
//                 crc = (crc << 1) ^ 0x07;
//             else
//                 crc <<= 1;
//         }
//     }
//     return crc;
// }

// void parse_usb_data(uint8_t byte)
// {
//     switch (parse_state)
//     {
//     case STATE_WAIT_HEADER:
//         if (byte == CMD_HEADER)
//         {
//             pbi = 0;
//             parse_state = STATE_WAIT_DATA_TYPE;
//             parse_buffer[pbi] = byte;
//             /*code*/
//         }
//         break;

//     case STATE_WAIT_DATA_TYPE:
//         if (byte == DATA_PC_CMD)
//         {
//             parse_state = STATE_WAIT_CMD;
//             parse_buffer[++pbi] = byte;
//             /*code*/
//         }
//         break;
//     case STATE_WAIT_CMD:
//         current_cmd = byte;
//         parse_buffer[++pbi] = byte;
//         parse_state = STATE_WAIT_LEN0;
//         break;
//     case STATE_WAIT_LEN0:
//         parse_buffer[++pbi] = byte;
//         expected_len0 = byte;
//         parse_state = STATE_WAIT_LEN1;
//         break;
//     case STATE_WAIT_LEN1:
//         expected_len1 = byte;
//         expected_len = ((expected_len0 << 8) & 0xFF00) | (expected_len1 & 0xFF);
//         parse_buffer[++pbi] = byte;
//         data_index = 0;

//         head_size = pbi + 1;

//         if (expected_len == 0)
//         {
//             parse_state = STATE_WAIT_CRC;
//         }
//         else if (expected_len <= OTG_BUFFER_SIZE - head_size)
//         {
//             parse_state = STATE_WAIT_DATA;
//         }
//         else
//         {
//             parse_state = STATE_WAIT_HEADER; // длина превышает буфер
//                                              // TODO отправить сигнал ошибки
//         }
//         break;

//     case STATE_WAIT_DATA:
//         parse_buffer[++pbi] = byte;
//         data_index++;

//         if (data_index >= expected_len)
//         {
//             parse_state = STATE_WAIT_CRC;
//         }
//         break;

//     case STATE_WAIT_CRC:
//     {
//         uint8_t crc_calc_result = crc8_calc(parse_buffer, head_size + expected_len);
//         if (crc_calc_result == byte)
//         {
//             process_command(current_cmd, &parse_buffer[head_size], expected_len);
//         }
//         else
//         {
//             // CRC ошибка — можно сигнализировать, если нужно
//         }
//         parse_state = STATE_WAIT_HEADER;
//         break;
//     }

//     default:
//         parse_state = STATE_WAIT_HEADER;
//         break;
//     }
// }

/*====================================================================*/

// static const cmd_handler_t cmd_table[256] =
//     {}; TODO add this

static inline uint16_t stream_available(USBStream *s)
{
    uint16_t head = s->head;
    uint16_t tail = s->tail;
    if (head >= tail)
        return head - tail;

    return PARSER_BUFFER_SIZE - tail + head;
}

static inline uint8_t stream_peek(USBStream *s, uint16_t offset)
{
    return s->buffer[(s->tail + offset) & (PARSER_BUFFER_SIZE - 1)];
}

static inline void stream_consume(USBStream *s, uint16_t count)
{
    s->tail = (s->tail + count) & (PARSER_BUFFER_SIZE - 1);
}

inline uint16_t stream_write(USBStream *s, const uint8_t *data, uint16_t len)
{
    uint16_t free;

    if (s->head >= s->tail)
        free = PARSER_BUFFER_SIZE - (s->head - s->tail) - 1;
    else
        free = s->tail - s->head - 1;

    if (len > free)
        len = free;

    uint16_t first = PARSER_BUFFER_SIZE - s->head;
    if (first > len)
        first = len;

    memcpy(&s->buffer[s->head], data, first);
    memcpy(&s->buffer[0], data + first, len - first);

    s->head = (s->head + len) & (PARSER_BUFFER_SIZE - 1);

    return len;
}

static uint8_t crc8_update(uint8_t crc, uint8_t data)
{
    crc ^= data;

    for (uint8_t i = 0; i < 8; i++)
    {
        if (crc & 0x80)
            crc = (crc << 1) ^ 0x07;
        else
            crc <<= 1;
    }

    return crc;
}

void parser_process(USBStream *s)
{
    if (!s)
        return;
    while (stream_available(s) >= MIN_PACKET_SIZE)
    {

        if (stream_peek(s, 0) != CMD_HEADER)
        {
            stream_consume(s, 1);
            continue;
        }

        StreamDataType type = stream_peek(s, 1);

        if (type != DATA_PC_CMD)
        {
            stream_consume(s, 1);
            continue;
        }

        // uint8_t cmd = stream_peek(s, 2);

        uint16_t len = ((uint16_t)stream_peek(s, 2) << 8) | stream_peek(s, 3);

        if (len == 0 || len > MAX_PAYLOAD)
        {
            stream_consume(s, 1); // ignore packet without payload
            continue;
        }

        uint16_t packet_size = HEADER_SIZE + len + CRC_SIZE;

        if (stream_available(s) < packet_size)
        {
            return; // wait for full packet
        }

        uint8_t crc = 0;

        for (uint16_t i = 0; i < packet_size - 1; i++)
        {
            crc = crc8_update(crc, stream_peek(s, i));
        }

        uint8_t received_crc = stream_peek(s, packet_size - 1);

        if (crc == received_crc)
        {
            uint8_t payload[MAX_PAYLOAD];
            for (uint16_t i = 0; i < len; i++)
            {
                payload[i] = stream_peek(s, HEADER_SIZE + i);
            }

            // process_command(payload, len); TODO add func

            stream_consume(s, packet_size);
        }
        else
        {
            // stream_consume(s, 1);
            while (stream_available(s) > 0 && stream_peek(s, 0) != CMD_HEADER)
            {
                stream_consume(s, 1);
            }
        }
    }
}