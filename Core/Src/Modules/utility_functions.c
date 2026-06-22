#include "utility_functions.h"

void DRDY_no_responce_timeout_handle(adc_context *ctx)
{
    if (ctx->DRDY_IsLow)
    {
        uint16_t timeout = (uint16_t)1000;
        while (timeout > 0U)
        {
            if (!ctx->DRDY_IsLow)
            {
                break;
            }
            --timeout;
        }
        if (timeout == 0U)
        {
            ctx->cs_port->BSRR = ctx->cs_pin;

            ctx->rx->CCR &= ~DMA_CCR_EN;
            ctx->tx->CCR &= ~DMA_CCR_EN;

            // Clear DMA flags
            ctx->dma->IFCR = ctx->tcif_tx_ch | ctx->teif_tx_ch | ctx->htif_tx_ch |
                             ctx->tcif_rx_ch | ctx->teif_rx_ch | ctx->htif_rx_ch;
            ctx->DRDY_IsLow = (bool)false;
        }
    }
}

void internal_DAC_EN_DIS_mgr(void)
{
    if (Latches.INTERNAL_DAC_IsLocked)
    {
        if (dac_running)
        {
            // HAL_TIM_Base_Stop(&htim6);
            if (HAL_TIM_Base_Stop(&htim6) != HAL_OK)
            {
                Error_Handler();
            }
            dac_running = (bool)false;
        }
    }
    else
    {
        if (!dac_running)
        {
            // HAL_TIM_Base_Start(&htim6);
            if (HAL_TIM_Base_Start(&htim6) != HAL_OK)
            {
                Error_Handler();
            }
            dac_running = (bool)true;
        }
    }
}

void internal_ADC_EN_DIS_mgr(void)
{
    if (Latches.INTERNAL_ADC_IsLocked)
    {
        if (adc_running)
        {
            // HAL_TIM_Base_Stop(&htim7);
            if (HAL_TIM_Base_Stop(&htim7) != HAL_OK)
            {
                Error_Handler();
            }
            adc_running = (bool)false;
        }
    }
    else
    {
        if (!adc_running)
        {
            if (HAL_TIM_Base_Start(&htim7) != HAL_OK)
            {
                Error_Handler();
            }

            // HAL_TIM_Base_Start(&htim7);
            adc_running = (bool)true;
        }
    }
}

Uplink_Packet create_packet(StreamDataType t, uint16_t len)
{
    Uplink_Packet s = {.dataType = t,
                        .length = len};
    return s;
}

void processAdcBatches(adc_context *ctx)
{
    if (*(ctx->batch_IsReady))
    {
        *(ctx->batch_IsReady) = (bool)false;
        __DMB();
        Uplink_Packet packet = create_packet(ctx->data_type, (uint16_t)(ADC_SAMPLES_THRESHOLD * ADC_SAMPLE_SIZE));

        for (uint32_t i = 0; i < ADC_SAMPLES_THRESHOLD; i++)
        {
            uint32_t idx = (ctx->adc_buf->tail + i) & (ADC_BUFFER_ELEMENTS - 1U);
            (void)memcpy(&packet.data[i * ADC_SAMPLE_SIZE], ctx->adc_buf->buffer[idx].data, ADC_SAMPLE_SIZE);
        }

        // advance tail
        ctx->adc_buf->tail = (ctx->adc_buf->tail + ADC_SAMPLES_THRESHOLD) & (ADC_BUFFER_ELEMENTS - 1U);

        pushPacket(ctx->uplink_stream, &packet);
    }
}

void processAdcBatches1(adc_context *ctx)
{
    if (*(ctx->batch_IsReady))
    {
        *(ctx->batch_IsReady) = (bool)false;
        __DSB();

        Uplink_USB_Stream *stream = ctx->uplink_stream;
        uint8_t head = stream->queueHead;
        uint8_t next = (uint8_t)((head + 1U) & (MAX_QUEUE - 1U));

        if (next == stream->queueTail) return;

        Uplink_Packet *qPacket = &(stream->packetQueue[head]);
        qPacket->dataType = ctx->data_type;
        qPacket->length = (uint16_t)(ADC_SAMPLES_THRESHOLD * 3); // Ровно 3 байта на сэмпл

        uint32_t rb_tail = ctx->adc_buf->tail;
        uint8_t *dst = qPacket->data;

        for (uint32_t i = 0; i < ADC_SAMPLES_THRESHOLD; i++)
        {
            uint32_t idx = (rb_tail + i) & (ADC_BUFFER_ELEMENTS - 1U);
            
            // Читаем 4 байта (выровнено и быстро)
            uint32_t sample = *(uint32_t*)(&ctx->adc_buf->buffer[idx]);
            // TODO проверить работоспособность такого решения
            // // Пишем по 1 байту (без пробелов и безопасно для памяти)
            // *dst++ = (uint8_t)(sample);         // Младший байт
            // *dst++ = (uint8_t)(sample >> 8);    // Средний байт
            // *dst++ = (uint8_t)(sample >> 16);   // Старший байт

            // Продвигаем указатель только на 3 байта!
            *(uint32_t*)dst = sample; 
            dst += 3;
        }

        ctx->adc_buf->tail = (rb_tail + ADC_SAMPLES_THRESHOLD) & (ADC_BUFFER_ELEMENTS - 1U);
        __atomic_store_n(&stream->queueHead, next, __ATOMIC_RELEASE);
    }
}

void GenerateSineWave(uint16_t *array)
{
    for (uint16_t i = 0U; i < SINE_WAVE_SAMPLES; i++)
    {
        double angle = (2.0 * 3.1415 * i) / SINE_WAVE_SAMPLES; //-V2568 //-V2568

        double sine = sin(angle);

        // shift from [-1,1] to [0,1]
        sine = (sine + 1.0) / 2.0;

        // scale to DAC range
        array[i] = (uint16_t)(sine * DAC_RESOLUTION);
    }
}