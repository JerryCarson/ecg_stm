#include "usb_parser.h"
#include "cmd_handler.h"
#include "ring_buffer.h"
#include "usbd_cdc_if.h"

static /*inline*/ uint16_t stream_available(USBStream *s) // TODO раскомментировать inline после отладки
{
    uint16_t head = s->head;
    uint16_t tail = s->tail;
    __DSB();
    if (head >= tail)
        return head - tail;

    return PARSER_BUFFER_SIZE - tail + head;
}

static inline uint8_t stream_peek(USBStream *s, uint16_t offset)
{
    __DMB();
    return s->buffer[(s->tail + offset) & (PARSER_BUFFER_SIZE - 1)];
}

static inline void stream_consume(USBStream *s, uint16_t count)
{
    __DSB();
    s->tail = (s->tail + count) & (PARSER_BUFFER_SIZE - 1);
}

inline uint16_t stream_write(USBStream *s, const uint8_t *data, uint16_t len)
{
    // ⚠️ Cache volatile reads ONCE at start
    uint16_t head = s->head;
    uint16_t tail = s->tail;

    // Calculate free space using cached values only
    uint16_t free;
    if (head >= tail)
        free = PARSER_BUFFER_SIZE - (head - tail) - 1;
    else
        free = tail - head - 1;

    if (len > free)
        len = free;

    if (len == 0)
        return 0; // Buffer full

    // Write data to buffer (wrap-around safe)
    uint16_t first = PARSER_BUFFER_SIZE - head;
    if (first > len)
        first = len;

    memcpy(&s->buffer[head], data, first);
    memcpy(&s->buffer[0], data + first, len - first);

    // ⚠️ Memory barrier: ensure buffer writes complete BEFORE head update becomes visible
    __DMB();

    // Update head (this is the ONLY writer of head)
    s->head = (head + len) & (PARSER_BUFFER_SIZE - 1);

    return len;
}

// static uint8_t crc8_update(uint8_t crc, uint8_t data)
// {
//     crc ^= data;

//     for (uint8_t i = 0; i < 8; i++)
//     {
//         if (crc & 0x80)
//             crc = (crc << 1) ^ 0x07;
//         else
//             crc <<= 1;
//     }

//     return crc;
// }

void USB_stream_data()
{
    StreamPacket_t *pkt = peekPacket();
    __DMB();
    if (pkt)
    {
        if (pkt->length > MAX_PACKET_SIZE)
        {
            consumePacket();
            return;
        }
        static uint8_t buf[MAX_PACKET_SIZE + HEADER_SIZE + CRC_SIZE];
        buf[0] = CMD_HEADER;
        buf[1] = pkt->dataType;
        buf[2] = (pkt->length) >> 8;
        buf[3] = (pkt->length) & 0xFF;
        for (size_t i = 0; i < pkt->length; i++)
        {
            buf[HEADER_SIZE + i] = pkt->data[i];
        }
        uint16_t total = HEADER_SIZE + pkt->length + CRC_SIZE - 1;

        CRC->CR = CRC_CR_RESET;

        for (uint16_t i = 0; i < total; i++)
        {
            *(volatile uint8_t *)&CRC->DR = buf[i]; // TODO ИСПРАВИТЬ CRC!!!
        }

        uint8_t crc = (uint8_t)CRC->DR;

        // uint8_t crc = 0;
        // for (uint16_t i = 0; i < total; i++)
        // {
        //     crc = crc8_update(crc, buf[i]); // TODO use hardware CRC calc
        // }

        buf[HEADER_SIZE + pkt->length] = crc;

        if (CDC_Transmit_FS(buf, pkt->length + HEADER_SIZE + CRC_SIZE) == USBD_OK)
        {
            consumePacket();
        }
    }
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

        // uint8_t crc = 0;

        // for (uint16_t i = 0; i < packet_size - 1; i++)
        // {
        //     crc = crc8_update(crc, stream_peek(s, i)); // TODO use hardware CRC calc
        // }

        /* 1. Set POLYSIZE to 8-bit
   RM0440: 8-bit mode = 10b at bits 4:3
   Your header: CRC_CR_POLYSIZE_1 = (0x2UL << 3) = 0x10 */
        CRC->CR = (CRC->CR & ~CRC_CR_POLYSIZE_Msk) | CRC_CR_POLYSIZE_1;

        /* 2. Configure CRC-8 parameters (adjust to your protocol spec) */
        CRC->POL = 0x07;  // Polynomial
        CRC->INIT = 0x00; // Initial CRC value
        // CRC->XOR = 0x00; // Final XOR (CMSIS names it XORDATA, not XOR)

        CRC->CR |= CRC_CR_RESET;
        __DSB();
        __ISB();

        // 2. Wait for hardware to clear the reset bit (safe practice)
        while (CRC->CR & CRC_CR_RESET)
            ;

        for (uint16_t i = 0; i < packet_size - 1; i++)
        {
            *(__IO uint8_t *)&CRC->DR = stream_peek(s, i);
        }

        uint8_t crc = (uint8_t)CRC->DR;

        uint8_t received_crc = stream_peek(s, packet_size - 1);

        volatile uint16_t sa = stream_available(s);

        if (crc == received_crc)
        {
            uint8_t payload[MAX_PAYLOAD];
            for (uint16_t i = 0; i < len; i++)
            {
                payload[i] = stream_peek(s, HEADER_SIZE + i);
            }
            process_command(payload, len);

            stream_consume(s, packet_size);
        }
        else
        {
            stream_consume(s, 1);

            while ((stream_available(s) > 0) && (stream_peek(s, 0) != CMD_HEADER))
            {
                stream_consume(s, 1);
            }
        }
    }
}