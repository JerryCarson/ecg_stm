#include "usb_parser.h"

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