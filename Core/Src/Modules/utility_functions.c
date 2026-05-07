#include "utility_functions.h"

void DRDY_no_responce_timeout_handle(adc_dma_context_t *ctx)
{
    if (ctx->DRDY_low)
    {
        uint16_t timeout = 1000;
        while ((ctx->DRDY_low) && (--timeout))
            ;
        if (!timeout)
        {
            ctx->cs_port->BSRR = ctx->cs_pin;

            ctx->rx->CCR &= ~DMA_CCR_EN;
            ctx->tx->CCR &= ~DMA_CCR_EN;

            // Clear DMA flags
            ctx->dma->IFCR = ctx->tcif_tx_ch | ctx->teif_tx_ch | ctx->htif_tx_ch |
                             ctx->tcif_rx_ch | ctx->teif_rx_ch | ctx->htif_rx_ch;
            ctx->DRDY_low = 0;
        }
    }
}

void internal_DAC_EN_DIS_mgr()
{
    if (Latches.INTERNAL_DAC_LOCK)
    {
        if (dac_running)
        {
            HAL_TIM_Base_Stop(&htim6);
            dac_running = false;
        }
    }
    else
    {
        if (!dac_running)
        {
            HAL_TIM_Base_Start(&htim6);
            dac_running = true;
        }
    }
}

void internal_ADC_EN_DIS_mgr()
{
    if (Latches.INTERNAL_ADC_LOCK)
    {
        if (adc_running)
        {
            HAL_TIM_Base_Stop(&htim7);
            adc_running = false;
        }
    }
    else
    {
        if (!adc_running)
        {
            HAL_TIM_Base_Start(&htim7);
            adc_running = true;
        }
    }
}

StreamPacket_t create_packet(StreamDataType t, uint16_t len)
{
    StreamPacket_t s = {.dataType = t,
                        .length = len};
    return s;
}

void processAdcBatches(adc_dma_context_t *ctx)
{
    if (ctx->batch_ready_flag)
    {
        ctx->batch_ready_flag = false;
        __DMB();
        StreamPacket_t packet = create_packet(ctx->data_type, ADC_BATCH_SIZE * ADC_BYTES_PER_SAMPLE);

        for (uint32_t i = 0; i < ADC_BATCH_SIZE; i++)
        {
            uint32_t idx = (ctx->buf->tail + i) & (ADC_BUFFER_ELEMENTS - 1);
            memcpy(&packet.data[i * ADC_BYTES_PER_SAMPLE], ctx->buf->buffer[idx].data, ADC_BYTES_PER_SAMPLE);
        }

        // advance tail
        ctx->buf->tail = (ctx->buf->tail + ADC_BATCH_SIZE) & (ADC_BUFFER_ELEMENTS - 1);

        pushPacket(ctx->uplink_stream, &packet);
    }
}

void GenerateSineWave(uint16_t *array)
{
    for (uint16_t i = 0; i < SINE_WAVE_SAMPLES; i++)
    {
        double angle = (2.0 * 3.1415 * i) / SINE_WAVE_SAMPLES;

        double sine = sin(angle);

        // shift from [-1,1] to [0,1]
        sine = (sine + 1.0) / 2.0;

        // scale to DAC range
        array[i] = (uint16_t)(sine * DAC_RESOLUTION);
    }
}