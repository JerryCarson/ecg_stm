/**
 * @file adc_handler.h
 * @brief Обработчик взаимодействия с внешними 24-битными АЦП ADS127L21 через SPI с использованием DMA.
 *
 * Данный модуль реализует:
 * - Обработку прерываний DRDY от АЦП;
 * - Работа с кольцевым буфером для хранения данных;
 * - Настройку и работу DMA для SPI с низкой задержкой;
 * - Логику формирования батчей данных для передачи по USB.
 *
 * Рассчитан на высокую скорость и низкую задержку.
 *
 * @addtogroup EXTERNAL_ADC_HANDLING Обработчик данных от внешнего ADC
 * @{
 */

#ifndef ADC_HANDLER_H
#define ADC_HANDLER_H

#include <stdbool.h>
#include "main.h"
#include "uplink_buffer.h"

/**
 * @def ADC_BYTES_PER_SAMPLE
 * @brief Размер пакета данных в байтах для общения с внешним ADC.
 */
#define ADC_BYTES_PER_SAMPLE 3

/**
 * @def ADC_BATCH_SIZE
 * @brief Порог количества сэмплов внешнего ADC в кольцевом буфере @ref AdcRingBuffer_t,
 * после которого значения из него отправляются в буфер @ref Uplink_USB_Stream для дальнейшей отправки на ПК.
 */
#define ADC_BATCH_SIZE 42 // Send 42 pairs per USB packet

/**
 * @def ADC_BUFFER_ELEMENTS
 * @brief Размер буфера @ref AdcRingBuffer_t который содержит сэмплы, полученные от внешнего ADC.
 */
#define ADC_BUFFER_ELEMENTS 512

/**
 * @def ADC_SETUP_REGS_COUNT
 * @brief Количество регистров настройки внешнего ADC
 */
#define ADC_SETUP_REGS_COUNT 5

_Static_assert((ADC_BUFFER_ELEMENTS & (ADC_BUFFER_ELEMENTS - 1)) == 0,
               "ADC_BUFFER_ELEMENTS must be power of two");

_Static_assert(ADC_BATCH_SIZE < MAX_PACKET_SIZE,
               "ADC_BATCH_SIZE must fit in StreamPacket_t max packet size");

/**
 * @brief Структура сэмпла внешнего ADC.
 * В составе кольцевого буфера @ref AdcRingBuffer_t хранит получаемые от внешних ADC сэмплов.
 */
typedef struct AdcSample_t
{
   uint8_t data[ADC_BYTES_PER_SAMPLE]; /**< Массив для хранения сэмпла */
} AdcSample_t;

/**
 * @brief Структура кольцевого буфера для хранения сэмплов внешнего АЦП.
 * Буферы @ref adc1_buf и @ref adc2_buf используются для хранения сэмплов от первого
 * и второго внешнего ADC соответственно.
 */
typedef struct AdcRingBuffer_t
{
   AdcSample_t buffer[ADC_BUFFER_ELEMENTS]; /**< Массив кольцевого буфера */
   volatile uint32_t head;                  /**< Указывает индекс головы буфера */
   volatile uint32_t tail;                  /**< Указывает индекс хвоста буфера */
} AdcRingBuffer_t;

/**
 * @brief Структура контекста для обработки прерываний по DRDY и по окончанию приема по DMA.
 * Содержит адреса регистров, указатели и переменные для конкретного SPI и DMA канала,
 * которые используются при работе с конкретным внешним ADC.
 */
typedef struct adc_dma_context_t
{
   DMA_TypeDef *dma;                 /**< Адрес периферии DMA */
   uint32_t teif_rx_ch;              /**< Флаг Transfer Error для канала RX*/
   uint32_t tcif_rx_ch;              /**< Флаг Transfer Complete для канала RX*/
   uint32_t htif_rx_ch;              /**< Флаг Half Transfer Complete для канала RX*/
   uint32_t teif_tx_ch;              /**< Флаг Transfer Error для канала TX*/
   uint32_t tcif_tx_ch;              /**< Флаг Transfer Complete для канала TX*/
   uint32_t htif_tx_ch;              /**< Флаг Half Transfer Complete для канала TX*/
   DMA_Channel_TypeDef *rx;          /**< Адрес RX DMA-канала */
   DMA_Channel_TypeDef *tx;          /**< Адрес TX DMA-канала */
   SPI_TypeDef *spi;                 /**< Адрес периферии SPI */
   GPIO_TypeDef *cs_port;            /**< Адрес порта, на котором находится пин CS */
   uint16_t cs_pin;                  /**< Адрес пина CS */
   GPIO_TypeDef *start_port;         /**< Адрес порта, на котором находится пин START */
   uint16_t start_pin;               /**< Адрес пина START */
   volatile uint32_t *error_count;   /**< Счетчик ошибок */
   AdcRingBuffer_t *buf;             /**< Указатель на кольцевой буфер */
   volatile uint32_t batch_count;    /**< Счетчик количества сэмплов в буфере @ref AdcRingBuffer_t */
   volatile bool *batch_ready_flag;  /**< Флаг достижения в буфере количества сэмплов, равного @ref ADC_BATCH_SIZE*/
   volatile uint8_t *spi_buf;        /**< Указатель на массив, в который поступают данные от DMA RX канала */
   volatile bool DRDY_low;           /**< Флаг срабатывания DRDY */
   StreamDataType data_type;         /**< Тип данных, привязанный к контексту, необходим для маркировки пакета данных при отправке на ПК */
   Uplink_USB_Stream *uplink_stream; /**< Указатель на буфер, из которого данные отправляются на ПК */
} adc_dma_context_t;

/** @brief Контекст для обработки прерываний от I ADC */
extern adc_dma_context_t adc1_ctx;

/** @brief Контекст для обработки прерываний от II ADC */
extern adc_dma_context_t adc2_ctx;

/**
 * @brief Счетчик ошибок первого внешнего ADC.
 * Инкрементируется при обнаружении ошибок SPI или DMA во время приема сэмплов
 * и используется для статистики и диагностики работы ADC.
 */
extern volatile uint32_t g_adc1_error_count;

/**
 * @brief Счетчик ошибок второго внешнего ADC.
 * Инкрементируется при обнаружении ошибок SPI или DMA во время приема сэмплов
 * и используется для статистики и диагностики работы ADC.
 */
extern volatile uint32_t g_adc2_error_count;

/**
 * @brief Флаг достижения ADC_BATCH_SIZE для первого ADC.
 * Устанавливается в true, когда кольцевой буфер adc1_buf накопил достаточное
 * количество сэмплов для формирования пакета StreamPacket_t. После обработки батча флаг сбрасывается.
 */
extern volatile bool adc1_batch_size_reached;

/**
 * @brief Флаг достижения ADC_BATCH_SIZE для второго ADC.
 * Устанавливается в true, когда кольцевой буфер adc2_buf накопил достаточное
 * количество сэмплов для формирования пакета StreamPacket_t. После обработки батча флаг сбрасывается.
 */
extern volatile bool adc2_batch_size_reached;

/**
 * @brief Буфер для приема данных по SPI с первого ADC.
 * Массив байтов, в который DMA RX канал записывает новые сэмплы.
 */
extern volatile uint8_t g_spi1_buf[];

/**
 * @brief Буфер для приема данных по SPI со второго ADC.
 * Массив байтов, в который DMA RX канал записывает новые сэмплы.
 */
extern volatile uint8_t g_spi2_buf[];

/**
 * @brief Массив пустых данных для передачи по SPI при чтении ADC.
 * Используется для генерации тактовых импульсов SPI без фактической передачи данных.
 * DMA TX канал считывает данные из этого массива при приеме сэмплов от ADC.
 */
extern const uint8_t SPI_DUMMY_TX[];

/**
 * @brief Кольцевой буфер первого ADC.
 * Хранит сэмплы, полученные с первого внешнего ADC.
 * Структура содержит массив сэмплов и индексы головы и хвоста для организации кольцевого буфера.
 */
extern AdcRingBuffer_t adc1_buf;

/**
 * @brief Кольцевой буфер второго ADC.
 * Хранит сэмплы, полученные со второго внешнего ADC.
 * Структура содержит массив сэмплов и индексы головы и хвоста для организации кольцевого буфера.
 */
extern AdcRingBuffer_t adc2_buf;

/**
 * @brief Выключает DMA каналы, включает оба SPI, включает TX и RX буферы для DMA, поднимает оба CS.
 */
void ADC_Handler_Init(void);

/**
 * @brief Настраивает регистры внешнего ADC
 * @param ctx Указатель на контекст @ref adc_dma_context_t конкретного ADC.
 */
void ADC_setup(adc_dma_context_t *ctx);

/**
 * @brief Добавляет новый сэмпл в кольцевой буфер.
 *
 * Функция безопасно обновляет индекс головы буфера. При заполнении буфера
 * новый сэмпл не добавляется, предотвращая перезапись непрочитанных данных.
 *
 * @param[in]  rb   Указатель на структуру кольцевого буфера.
 * @param[in]  data Указатель на массив данных сэмпла размером @ref ADC_BYTES_PER_SAMPLE байт.
 * @retval true     Сэмпл успешно записан в буфер.
 * @retval false    Буфер заполнен, запись отменена.
 *
 * @note Вызывается из контекста ISR/DMA @ref adc_dma_isr. Не содержит блокирующих операций и аллокаций памяти.
 */
FORCE_INLINE bool adc_push(AdcRingBuffer_t *rb, volatile uint8_t *data)
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

/**
 * @brief Отправляет массив данных по SPI интерфейсу, используется для взаимодействия
 * с внешними ADC.
 * @param ctx Указатель на контекст @ref adc_dma_context_t конкретного ADC.
 * @param tx_buf Указатель на массив отправляемых данных.
 * @param rx_buf Указатель на массив принимаемых данных.
 * @param len Длина обоих массивов.
 * @param uses_rx_cplt_interrupt Выставляется true если функция вызвана из
 * DRDY-прерывания, в таком случае функция становится не блокирующей и обработка
 * полученных пакетов происходит в SPI DMA RX прерывании.
 * Требуется для корректной обработки данных от внешнего ADC. Если требуется
 * просто отправить данные - устанавливается в false.
 */
void SPI_DMA_TX_RX_byte_array(adc_dma_context_t *ctx,
                              const uint8_t *tx_buf,
                              volatile uint8_t *rx_buf,
                              uint8_t len,
                              bool uses_rx_cplt_interrupt);

/**
 * @brief Оптимизированный вариант функции для отправки данных по SPI.
 * * Уменьшено количество аргументов во избежание траты тактов на запись части аргументов в стек
 * * Добавлен спецификатор inline во избежание траты тактов на вызов функции
 *
 * Используется для работы в обработчике прерывания.
 * @param ctx Указатель на контекст @ref adc_dma_context_t конкретного ADC.
 * @param tx_buf Указатель на массив отправляемых данных.
 * @param rx_buf Указатель на массив принимаемых данных.
 */
FORCE_INLINE void SPI_DMA_TX_RX_byte_array_isr(adc_dma_context_t *ctx, // TODO На всякий случай перепроверить на реентрантность
                                               const uint8_t *tx_buf,
                                               volatile uint8_t *rx_buf)
{
   /* Check if DMA still active */
   if ((ctx->rx->CCR & DMA_CCR_EN) || (ctx->tx->CCR & DMA_CCR_EN)) // TODO возможно нужно && вместо ||
   {
      ctx->error_count++;
      return;
   }

   // Disable DMA channels
   ctx->rx->CCR &= ~DMA_CCR_EN;
   ctx->tx->CCR &= ~DMA_CCR_EN;
   __DSB();
   ctx->tx->CPAR = (uint32_t)&ctx->spi->DR; // Peripheral address is SPI data register
   ctx->rx->CPAR = (uint32_t)&ctx->spi->DR; // TODO Попробовать переместить настройку CPAR регистров в Init-функцию для ускорения работы
   // __DSB();
   ctx->tx->CMAR = (uint32_t)tx_buf; // Memory address of TX buffer
   ctx->tx->CNDTR = 3;               // Number of bytes to transfer
   // __DSB();
   ctx->rx->CMAR = (uint32_t)rx_buf; // Memory address of RX buffer
   ctx->rx->CNDTR = 3;               // Number of bytes to transfer
   // __DSB();

   // Clear pending DMA flags
   ctx->dma->IFCR = ctx->tcif_tx_ch | ctx->teif_tx_ch | ctx->htif_tx_ch |
                    ctx->tcif_rx_ch | ctx->teif_rx_ch | ctx->htif_rx_ch;

   // Pull CS LOW
   ctx->cs_port->BSRR = (uint32_t)ctx->cs_pin << 16U;
   __DSB();

   // Enable DMA channels
   ctx->rx->CCR |= DMA_CCR_EN;
   ctx->tx->CCR |= DMA_CCR_EN;
}

/**
 * @brief Обработчик прерывания DRDY внешнего ADC.
 * Функция вызывается из ISR при срабатывании линии DRDY.
 * - Проверяет состояние DMA RX канала.
 * - Сбрасывает флаги DMA и SPI.
 * - Настраивает DMA для приёма сэмпла.
 * - Активирует SPI и пин CS для выбранного ADC.
 *
 * При обнаружении ошибки увеличивает счётчик ошибок в @ref adc_dma_context_t.
 *
 * @param ctx Указатель на контекст @ref adc_dma_context_t конкретного ADC.
 */
FORCE_INLINE void ADC_DRDY_ISR(adc_dma_context_t *ctx)
{
   ctx->DRDY_low = true;
   SPI_DMA_TX_RX_byte_array_isr(ctx, SPI_DUMMY_TX, ctx->spi_buf);
}

/**
 * @brief Обработчик прерывания DMA внешнего ADC.
 * Функция вызывается при прерывании DMA RX/TX каналов.
 * Выполняет:
 * - Обработку ошибок передачи (Transfer Error).
 * - Завершение передачи (Transfer Complete) с безопасной остановкой DMA.
 * - Освобождение пина CS для ADC.
 * - Считывание данных из SPI.
 * - Добавление сэмпла в кольцевой буфер через @ref adc_push.
 * - Логику формирования батча: если @ref ADC_BATCH_SIZE достигнут, выставляет флаг готовности.
 *
 * В случае переполнения буфера или ошибок SPI/DMA увеличивает счетчик ошибок.
 *
 * @param ctx Указатель на контекст @ref adc_dma_context_t конкретного ADC.
 */
FORCE_INLINE void adc_dma_isr(adc_dma_context_t *ctx) // TODO Перепроверить на реентрантность
{
   ctx->DRDY_low = false;
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
      if (!adc_push(ctx->buf, ctx->spi_buf))
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
/** @} */
#endif /* ADC_HANDLER_H */