#include "downlink_parser.h"
#include "cmd_handler.h"
// #include "uplink_buffer.h"

static inline uint32_t stream_available(Downlink_USB_Stream *s) //-V2506
{
    const uint32_t head = (uint32_t)__atomic_load_n(&(s->head), __ATOMIC_ACQUIRE);
    const uint32_t tail = (uint32_t)__atomic_load_n(&(s->tail), __ATOMIC_RELAXED);
    // __ISB();
    if (head >= tail)
    {
        return head - tail;
    }

    return PARSER_BUFFER_SIZE - tail + head;
}

static inline uint8_t stream_peek(Downlink_USB_Stream *s, uint32_t offset)
{
    // __DMB();
    const uint32_t idx = (s->tail + offset) & (PARSER_BUFFER_SIZE - 1U); // TODO проверить работоспособность
    return s->buffer[idx];
    // return s->buffer[(s->tail + offset) & (PARSER_BUFFER_SIZE - 1U)];
}

static inline void stream_consume(Downlink_USB_Stream *s, uint32_t count)
{
    s->tail = (s->tail + count) & (PARSER_BUFFER_SIZE - 1U);
}

void parse_downlink_data(Downlink_USB_Stream *s) //-V2506
{
    if (!s)
    {
        return;
    }
    while (stream_available(s) >= MIN_PACKET_SIZE)
    {

        if (stream_peek(s, 0U) != CMD_HEADER)
        {
            stream_consume(s, 1U);
            continue;
        }

        StreamDataType type = (StreamDataType)stream_peek(s, 1U);

        if (type != DATA_PC_CMD)
        {
            stream_consume(s, 1);
            continue;
        }

        // uint8_t cmd = stream_peek(s, 2);

        // uint16_t len = (uint16_t)((uint16_t)(stream_peek(s, 2U) << 8U) | stream_peek(s, 3U));
        uint16_t len = (uint16_t)(((uint16_t)stream_peek(s, 2U) << 8U) | (uint16_t)stream_peek(s, 3U));

        if ((len == 0U) || (len > MAX_PAYLOAD))
        {
            stream_consume(s, 1U); // ignore packet without payload
            continue;
        }

        uint16_t packet_size = (uint16_t)(HEADER_SIZE + len + CRC_SIZE);

        if (stream_available(s) < packet_size)
        {
            return; // wait for full packet
        }

        // 1. Set POLYSIZE to 8-bit. RM0440: 8-bit mode = 10b at bits 4:3
        CRC->CR = (CRC->CR & ~CRC_CR_POLYSIZE_Msk) | CRC_CR_POLYSIZE_1;

        /* 2. Configure CRC-8 parameters (adjust to your protocol spec) */
        CRC->POL = 0x07U;  // Polynomial
        CRC->INIT = 0x00U; // Initial CRC value
        // CRC->XOR = 0x00; // Final XOR (CMSIS names it XORDATA, not XOR)

        CRC->CR |= CRC_CR_RESET;
        // __DSB(); //TODO проверить работоспособность
        // __ISB();

        // 2. Wait for hardware to clear the reset bit (safe practice)
        while ((CRC->CR & CRC_CR_RESET) != 0U)
        {
        };

        for (uint16_t i = 0U; i < packet_size - 1U; i++)
        {
            *(__IO uint8_t *)&CRC->DR = stream_peek(s, i);
        }

        uint8_t crc = (uint8_t)CRC->DR;

        uint8_t received_crc = stream_peek(s, packet_size - 1U);

        if (crc == received_crc)
        {
            uint8_t payload[MAX_PAYLOAD];
            for (uint16_t i = 0U; i < len; i++)
            {
                payload[i] = stream_peek(s, HEADER_SIZE + i);
            }
            process_command(payload, len);

            stream_consume(s, packet_size);
        }
        else
        {
            stream_consume(s, 1U);

            while (stream_available(s) > 0U)
            {
                if (stream_peek(s, 0U) == CMD_HEADER)
                {
                    break; // Заголовок найден
                }
                stream_consume(s, 1U);
            }
        }
    }
}