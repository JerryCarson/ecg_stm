#include "main.h"
#include "adc_handler.h"
#include <stdbool.h>
// #include "stm32g4xx_dmamux.h"

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

        .start_port = START_1_GPIO_Port,
        .start_pin = START_1_Pin,

        .error_count = &g_adc1_error_count,

        .buf = &adc1_buf,

        .batch_count = 0,
        .batch_ready_flag = &adc1_batch_size_reached,
        .spi_buf = g_spi1_buf,
        .DRDY_low = 0,
        .data_type = DATA_SPI_1,
        .uplink_stream = &EXT_ADC1_Stream};

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

        .start_port = START_2_GPIO_Port,
        .start_pin = START_2_Pin,

        .error_count = &g_adc2_error_count,

        .buf = &adc2_buf,

        .batch_count = 0,
        .batch_ready_flag = &adc2_batch_size_reached,
        .spi_buf = g_spi2_buf,
        .DRDY_low = 0,
        .data_type = DATA_SPI_2,
        .uplink_stream = &EXT_ADC2_Stream};

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

    DMA2_Channel1->CCR |= DMA_CCR_TCIE;
    DMA1_Channel2->CCR |= DMA_CCR_TCIE;

    SPI1->CR1 |= SPI_CR1_SPE;
    SPI2->CR1 |= SPI_CR1_SPE;

    SPI1->CR2 |= SPI_CR2_RXDMAEN | SPI_CR2_TXDMAEN;
    SPI2->CR2 |= SPI_CR2_RXDMAEN | SPI_CR2_TXDMAEN;

    /* Set CS Pins High (Inactive) */
    CS_1_GPIO_Port->BSRR = (uint32_t)CS_1_Pin;
    CS_2_GPIO_Port->BSRR = (uint32_t)CS_2_Pin;
}

void SPI_DMA_TX_RX_byte_array(adc_dma_context_t *ctx,
                              const uint8_t *tx_buf,
                              volatile uint8_t *rx_buf,
                              uint8_t len,
                              bool uses_rx_cplt_interrupt)
{
    /* Check if DMA still active */
    if ((ctx->rx->CCR & DMA_CCR_EN) & (ctx->tx->CCR & DMA_CCR_EN))
    {
        ctx->error_count++;
        return;
    }

    // Disable DMA channels
    ctx->rx->CCR &= ~DMA_CCR_EN;
    ctx->tx->CCR &= ~DMA_CCR_EN;
    __DSB();
    ctx->tx->CPAR = (uint32_t)&ctx->spi->DR; // Peripheral address is SPI data register
    ctx->rx->CPAR = (uint32_t)&ctx->spi->DR;
    __DSB();
    ctx->tx->CMAR = (uint32_t)tx_buf; // Memory address of TX buffer
    ctx->tx->CNDTR = len;             // Number of bytes to transfer
    __DSB();
    ctx->rx->CMAR = (uint32_t)rx_buf; // Memory address of RX buffer
    ctx->rx->CNDTR = len;             // Number of bytes to transfer
    __DSB();

    // Clear pending DMA flags
    ctx->dma->IFCR = ctx->tcif_tx_ch | ctx->teif_tx_ch | ctx->htif_tx_ch |
                     ctx->tcif_rx_ch | ctx->teif_rx_ch | ctx->htif_rx_ch;

    // Pull CS LOW
    uint8_t del = 100;
    ctx->cs_port->BSRR = (uint32_t)ctx->cs_pin << 16U;
    __DSB();
    // for (size_t i = 0; i < del; i++)
    // {
    //     __NOP();
    // }
    // __DSB();
    // __ISB();
    // Enable DMA channels
    ctx->rx->CCR |= DMA_CCR_EN;
    ctx->tx->CCR |= DMA_CCR_EN;

    if (uses_rx_cplt_interrupt)
    {
        // If using RX complete interrupt, just return and let the ISR handle the rest
        return;
    }

    while ((!(ctx->dma->ISR & ctx->tcif_tx_ch)) && !(ctx->dma->ISR & ctx->tcif_rx_ch))
        ;

    while ((ctx->spi->SR & SPI_SR_BSY))
        ;

    __DSB();
    // __ISB();
    // for (size_t i = 0; i < del; i++)
    // {
    //     __NOP();
    // }
    // __DSB();
    // __ISB();
    // Pull CS HIGH
    ctx->cs_port->BSRR = ctx->cs_pin;

    ctx->rx->CCR &= ~DMA_CCR_EN;
    ctx->tx->CCR &= ~DMA_CCR_EN;

    // Clear DMA flags
    ctx->dma->IFCR = ctx->tcif_tx_ch | ctx->teif_tx_ch | ctx->htif_tx_ch |
                     ctx->tcif_rx_ch | ctx->teif_rx_ch | ctx->htif_rx_ch;
}

const uint16_t ADC_setup_regs[] =
    {
        // 0x0260U,
        0x0358U,
        0x0400U,
        0x0510U,
        0x0610U,
        0x0707U,
        0x0840U};

void ADC_setup(adc_dma_context_t *ctx)
{
    static uint8_t tx_buf[2];   // TX buffer for 1 register
    static uint8_t rx_dummy[2]; // dummy RX buffer

    // ctx->start_port->BSRR = ctx->start_pin;
    // ctx->start_port->BSRR = (uint32_t)ctx->start_pin << 16U; // Pull START LOW

    // Disable EXTI interrupts to prevent DRDY ISR firing
    NVIC_DisableIRQ(EXTI4_IRQn);
    NVIC_DisableIRQ(EXTI15_10_IRQn);
    NVIC_DisableIRQ(DMA1_Channel2_IRQn);
    NVIC_DisableIRQ(DMA2_Channel1_IRQn);

    // Stop DMA channels if running
    ctx->rx->CCR &= ~DMA_CCR_EN;
    ctx->tx->CCR &= ~DMA_CCR_EN;
    uint8_t del = 100;

    for (uint16_t i = 0; i < sizeof(ADC_setup_regs) / sizeof(*ADC_setup_regs); i++)
    {
        // Disable DMA channels
        ctx->rx->CCR &= ~DMA_CCR_EN;
        ctx->tx->CCR &= ~DMA_CCR_EN;
        __DSB();
        ctx->tx->CPAR = (uint32_t)&ctx->spi->DR; // Peripheral address is SPI data register
        ctx->rx->CPAR = (uint32_t)&ctx->spi->DR;
        __DSB();
        // Split 16-bit register into MSB/LSB
        tx_buf[0] = ((ADC_setup_regs[i] >> 8) & 0xFF) + 0x80; // 0x80 sets WRITE operation
        tx_buf[1] = ADC_setup_regs[i] & 0xFF;

        // Set DMA addresses and counts for this 2-byte transfer
        ctx->tx->CMAR = (uint32_t)tx_buf;
        ctx->tx->CNDTR = 2;
        __DSB();
        ctx->rx->CMAR = (uint32_t)rx_dummy;
        ctx->rx->CNDTR = 2;
        __DSB();
        // 4️⃣ Clear pending DMA flags
        ctx->dma->IFCR = ctx->tcif_tx_ch | ctx->teif_tx_ch | ctx->htif_tx_ch |
                         ctx->tcif_rx_ch | ctx->teif_rx_ch | ctx->htif_rx_ch;

        // Clear SPI flags properly (read, don't write)
        (void)ctx->spi->SR;
        (void)ctx->spi->DR; // Flush any stale data

        // Pull CS LOW for this register
        ctx->cs_port->BSRR = (uint32_t)ctx->cs_pin << 16U;

        __DSB();
        // for (size_t j = 0; j < del; j++)
        // {
        //     __NOP();
        // }
        // __DSB();
        // __ISB();

        // HAL_Delay(1);
        __DSB();
        // Enable DMA channels
        ctx->rx->CCR |= DMA_CCR_EN;
        ctx->tx->CCR |= DMA_CCR_EN;

        // *(volatile uint8_t *)&ctx->spi->DR = 0x00;

        while ((!(ctx->dma->ISR & ctx->tcif_tx_ch)) && !(ctx->dma->ISR & ctx->tcif_rx_ch))
            ;

        while ((!(ctx->dma->ISR & ctx->tcif_tx_ch)) && !(ctx->dma->ISR & ctx->tcif_rx_ch))
            ;

        // 2️⃣ Wait for SPI transmit buffer/FIFO to empty (TXE)
        // timeout = 10000;
        while (!(ctx->spi->SR & SPI_SR_TXE))
            ;

        while ((ctx->spi->SR & SPI_SR_BSY))
            ;

        // Pull CS HIGH
        // HAL_Delay(10);
        // __NOP();

        __DSB();
        // __ISB();
        // for (size_t j = 0; j < del; j++)
        // {
        //     __NOP();
        // }
        // __DSB();
        // __ISB();

        ctx->cs_port->BSRR = ctx->cs_pin;

        // Clear DMA flags
        ctx->dma->IFCR = ctx->tcif_tx_ch | ctx->teif_tx_ch | ctx->htif_tx_ch |
                         ctx->tcif_rx_ch | ctx->teif_rx_ch | ctx->htif_rx_ch;
    }

    ctx->rx->CCR &= ~DMA_CCR_EN;
    ctx->tx->CCR &= ~DMA_CCR_EN;

    // Re-enable EXTI interrupts
    NVIC_EnableIRQ(EXTI4_IRQn);
    NVIC_EnableIRQ(EXTI15_10_IRQn);
    NVIC_EnableIRQ(DMA1_Channel2_IRQn);
    NVIC_EnableIRQ(DMA2_Channel1_IRQn);

    // ctx->start_port->BSRR = (uint32_t)ctx->start_pin << 16U; // Pull START LOW
    // HAL_Delay(100);
    // ctx->start_port->BSRR = ctx->start_pin;
}