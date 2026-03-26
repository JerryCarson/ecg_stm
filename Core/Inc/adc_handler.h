/**
 * @file adc_handler.h
 * @brief Обработчик работы с внешними 24-битными АЦП ADS127L21 через SPI с использованием DMA.
 *
 * Данный модуль реализует:
 * - Обработку прерываний DRDY от АЦП;
 * - Работа с кольцевым буфером для хранения данных;
 * - Настройку и работу DMA для SPI с низкой задержкой;
 * - Логику формирования батчей данных для передачи по USB.
 *
 * Рассчитан на высокую скорость и низкую задержку.
 *
 * @note Для корректной работы предполагается, что все периферийные модули
 * и прерывания настроены через STM32CubeMX.
 */


#ifndef ADC_HANDLER_H
#define ADC_HANDLER_H

// #include <stdint.h>
#include <stdbool.h>
#include "main.h"

/* Timeout for DRDY2 (Approx 2us at 170MHz) */
#define DRDY2_TIMEOUT_CYCLES 340U

#define ADC_BYTES_PER_SAMPLE 3 // 24-bit
#define ADC_PAIR_BYTES (ADC_BYTES_PER_SAMPLE * 2U)
#define ADC_BATCH_SIZE 50       // Send 50 pairs per USB packet
#define ADC_BUFFER_ELEMENTS 256 // Ring buffer size (pairs)

_Static_assert((ADC_BUFFER_ELEMENTS & (ADC_BUFFER_ELEMENTS - 1)) == 0,
               "ADC_BUFFER_ELEMENTS must be power of two");

_Static_assert(ADC_BATCH_SIZE < MAX_PACKET_SIZE,
               "ADC_BATCH_SIZE must fit in StreamPacket_t max packet size");

typedef struct
{
   uint8_t data[3];
} AdcSample_t;

typedef struct
{
   AdcSample_t buffer[ADC_BUFFER_ELEMENTS];
   volatile uint32_t head;
   volatile uint32_t tail;
} AdcRingBuffer_t;

typedef struct adc_dma_context_t
{
   DMA_TypeDef *dma;
   uint32_t teif_rx_ch;
   uint32_t tcif_rx_ch;
   uint32_t htif_rx_ch;

   uint32_t teif_tx_ch;
   uint32_t tcif_tx_ch;
   uint32_t htif_tx_ch;

   DMA_Channel_TypeDef *rx;
   DMA_Channel_TypeDef *tx;

   SPI_TypeDef *spi;

   GPIO_TypeDef *cs_port;
   uint16_t cs_pin;

   volatile uint32_t *error_count;

   AdcRingBuffer_t *ring;

   volatile uint32_t batch_count;
   volatile bool *batch_ready_flag;
   volatile uint8_t *spi_buf;

} adc_dma_context_t;

extern adc_dma_context_t adc1_ctx;
extern adc_dma_context_t adc2_ctx;

// extern volatile uint32_t g_adc_error_count;
extern volatile uint32_t g_adc1_error_count;
extern volatile uint32_t g_adc2_error_count;
// extern volatile bool batch_size_reached;
extern volatile bool adc1_batch_size_reached;
extern volatile bool adc2_batch_size_reached;

extern volatile uint8_t g_spi1_buf[];
extern volatile uint8_t g_spi2_buf[];
extern const uint8_t SPI_DUMMY_TX[];

extern AdcRingBuffer_t adc1_buf;
extern AdcRingBuffer_t adc2_buf;

void ADC_Handler_Init(void);

static inline bool adc_push(AdcRingBuffer_t *rb, volatile uint8_t *data)
{
   uint32_t head = rb->head;
   uint32_t next = (head + 1) & (ADC_BUFFER_ELEMENTS - 1);

   uint32_t tail = rb->tail;
   if (next == tail)
      return false;

   rb->buffer[head].data[0] = data[0];
   rb->buffer[head].data[1] = data[1];
   rb->buffer[head].data[2] = data[2];

   __DMB();
   rb->head = next;

   return true;
}

static inline void ADC_DRDY_ISR(adc_dma_context_t *ctx)
{ 
   /* Check if DMA still active */
   if (ctx->rx->CCR & DMA_CCR_EN)
   {
      g_adc1_error_count++;
      return;
   }

   /* Clear DMA flags */
   ctx->dma->IFCR =
       ctx->tcif_rx_ch | ctx->teif_rx_ch | ctx->htif_rx_ch |
       ctx->tcif_tx_ch | ctx->teif_tx_ch | ctx->htif_tx_ch;

   /* Optional: clear SPI state */
   if (ctx->spi->SR & SPI_SR_OVR)
   {
      (void)ctx->spi->DR;
      (void)ctx->spi->SR;
      ctx->spi->CR1 &= ~SPI_CR1_SPE; // Disable SPI
      ctx->spi->CR1 |= SPI_CR1_SPE;  // Re-enable SPI
      ctx->error_count++;
   }

   /* CS LOW */
   __DMB();

   /* Configure DMA */
   ctx->rx->CMAR = (uint32_t)ctx->spi_buf;
   ctx->rx->CNDTR = 3;
   __DMB();
   ctx->tx->CMAR = (uint32_t)SPI_DUMMY_TX;
   ctx->tx->CNDTR = 3;
   __DMB();
   ctx->cs_port->BSRR = (uint32_t)ctx->cs_pin << 16U;
   __DMB();
   /* Enable RX first */
   ctx->rx->CCR |= DMA_CCR_EN;
   __DMB();
   ctx->tx->CCR |= DMA_CCR_EN;
}

static inline void adc_dma_isr(adc_dma_context_t *ctx) // TODO clear TX channels flags, Fix CS/DMA ordering
{
   uint32_t isr = ctx->dma->ISR;

   /* --- Transfer Error --- */
   if (isr & ctx->teif_rx_ch)
   {
      ctx->dma->IFCR = ctx->teif_rx_ch;

      ctx->tx->CCR &= ~DMA_CCR_EN;
      ctx->rx->CCR &= ~DMA_CCR_EN;

      if (ctx->spi->SR & SPI_SR_OVR)
      {
         (void)ctx->spi->DR;
         (void)ctx->spi->SR;
         ctx->spi->CR1 &= ~SPI_CR1_SPE; // Disable SPI
         ctx->spi->CR1 |= SPI_CR1_SPE;  // Re-enable SPI
      }

      (*ctx->error_count)++;
      return;
   }

   /* --- Transfer Complete --- */
   if (isr & ctx->tcif_rx_ch)
   {
      ctx->dma->IFCR = ctx->tcif_rx_ch;

      /* Wait until SPI fully finished */
      uint32_t timeout = 1000;
      while ((ctx->spi->SR & SPI_SR_BSY) && --timeout)
         ;
      if (!timeout)
      {
         (*ctx->error_count)++;
         return;
      }
      __DMB();

      /* Stop DMA safely */
      ctx->rx->CCR &= ~DMA_CCR_EN;
      ctx->tx->CCR &= ~DMA_CCR_EN;

      ctx->cs_port->BSRR = ctx->cs_pin; // release ADC ASAP
      __DMB();

      (void)ctx->spi->DR;
      (void)ctx->spi->SR;

      /* Push sample */
      if (!adc_push(ctx->ring, ctx->spi_buf))
      {
         (*ctx->error_count)++;
      }

      /* Batch logic */
      ctx->batch_count++;
      if (ctx->batch_count >= ADC_BATCH_SIZE)
      {
         *(ctx->batch_ready_flag) = true;
         ctx->batch_count = 0;
      }
   }
}

#endif /* ADC_HANDLER_H */