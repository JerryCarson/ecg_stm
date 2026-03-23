#include "main.h"
#include "adc_handler.h"
#include <stdbool.h>

volatile bool adc1_batch_size_reached = false;
volatile bool adc2_batch_size_reached = false;

// TODO 3-byte DMA buffer alignment is dangerous
//  volatile uint8_t g_spi1_buf[4] __attribute__((aligned(4)));
//  and ignore last byte.
volatile uint8_t g_spi1_buf[4] __attribute__((aligned(4)));
volatile uint8_t g_spi2_buf[4] __attribute__((aligned(4)));
const uint8_t SPI_DUMMY_TX[] = {0x00, 0x00, 0x00, 0x00};
volatile uint32_t g_adc1_error_count = 0;
volatile uint32_t g_adc2_error_count = 0;

AdcRingBuffer_t adc1_buf;
AdcRingBuffer_t adc2_buf;

adc_dma_context_t adc1_ctx =
    {
        .dma = DMA1,
        .teif_rx_ch = DMA_ISR_TEIF2,
        .tcif_rx_ch = DMA_ISR_TCIF2,
        .htif_rx_ch = DMA_ISR_HTIF2,

        .teif_tx_ch = DMA_ISR_TEIF3,
        .tcif_tx_ch = DMA_ISR_TCIF3,
        .htif_tx_ch = DMA_ISR_HTIF3,

        .rx = DMA1_Channel2,
        .tx = DMA1_Channel3,

        .spi = SPI1,

        .cs_port = CS_1_GPIO_Port,
        .cs_pin = CS_1_Pin,

        .error_count = &g_adc1_error_count,

        .ring = &adc1_buf,

        .batch_count = 0,
        .batch_ready_flag = &adc1_batch_size_reached,
        .spi_buf = g_spi1_buf};

adc_dma_context_t adc2_ctx =
    {
        .dma = DMA2,
        .teif_rx_ch = DMA_ISR_TEIF1,
        .tcif_rx_ch = DMA_ISR_TCIF1,
        .htif_rx_ch = DMA_ISR_HTIF1,

        .teif_tx_ch = DMA_ISR_TEIF2,
        .tcif_tx_ch = DMA_ISR_TCIF2,
        .htif_tx_ch = DMA_ISR_HTIF2,

        .rx = DMA2_Channel1,
        .tx = DMA2_Channel2,

        .spi = SPI2,

        .cs_port = CS_2_GPIO_Port,
        .cs_pin = CS_2_Pin,

        .error_count = &g_adc2_error_count,

        .ring = &adc2_buf,

        .batch_count = 0,
        .batch_ready_flag = &adc2_batch_size_reached,
        .spi_buf = g_spi2_buf};

void ADC_Handler_Init(void)
{
    /*
       CubeMX should have initialized SPI, DMA, and GPIO.
       We only ensure DMA is disabled initially so we can control it manually.
    */

    /* Disable DMA Channels initially */
    DMA1_Channel2->CCR &= ~DMA_CCR_EN; // SPI1 RX
    DMA1_Channel3->CCR &= ~DMA_CCR_EN; // SPI1 TX
    DMA2_Channel1->CCR &= ~DMA_CCR_EN; // SPI2 RX
    DMA2_Channel2->CCR &= ~DMA_CCR_EN; // SPI2 TX

    SPI1->CR1 |= SPI_CR1_SPE;
    SPI2->CR1 |= SPI_CR1_SPE;

    SPI1->CR2 |= SPI_CR2_RXDMAEN | SPI_CR2_TXDMAEN;
    SPI2->CR2 |= SPI_CR2_RXDMAEN | SPI_CR2_TXDMAEN;

    /* Set CS Pins High (Inactive) */
    CS_1_GPIO_Port->BSRR = (uint32_t)CS_1_Pin;
    CS_2_GPIO_Port->BSRR = (uint32_t)CS_2_Pin;
}
