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