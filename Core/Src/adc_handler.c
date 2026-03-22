#include "adc_handler.h"
#include "main.h" /* For HAL_GPIO_WritePin if needed, but we use registers */

// UsbADCRingBuffer_t g_usb_ring_buffer = {.head = 0, .tail = 0};

/* ISR Working Buffers (Keep these small and fast) */

volatile bool batch_size_reached = 0;

/*=====================================================================*/

volatile bool adc1_batch_size_reached = false;
volatile bool adc2_batch_size_reached = false;

//TODO 3-byte DMA buffer alignment is dangerous
// volatile uint8_t g_spi1_buf[4] __attribute__((aligned(4)));
// and ignore last byte.
volatile uint8_t g_spi1_buf[3] __attribute__((aligned(4)));
volatile uint8_t g_spi2_buf[3] __attribute__((aligned(4)));
const uint8_t SPI_DUMMY_TX[] = {0x00, 0x00, 0x00};
volatile uint32_t g_adc1_error_count = 0;
volatile uint32_t g_adc2_error_count = 0;

AdcRingBuffer_t adc1_buf;
AdcRingBuffer_t adc2_buf;

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

//TODO need this
// uint32_t USB_Buffer_Get_Data_Count(void)
// {
//     uint32_t head = g_usb_ring_buffer.head;
//     uint32_t tail = g_usb_ring_buffer.tail;
//     if (head >= tail)
//     {
//         return head - tail;
//     }
//     else
//     {
//         return USB_BUFFER_ELEMENTS - (tail - head);
//     }
// }

/*=====================================================================*/

// bool EXT_ADC_Buffer_Push(AdcPair_t *data)
// {
//     uint32_t next_head = (g_usb_ring_buffer.head + 1) & (USB_BUFFER_ELEMENTS - 1);

//     if (next_head == g_usb_ring_buffer.tail)
//     {
//         return false; // Overflow
//     }

//     g_usb_ring_buffer.buffer[g_usb_ring_buffer.head] = *data;
//     __DMB(); // Memory Barrier

//     g_usb_ring_buffer.head = next_head;
//     batch_size_reached = (USB_Buffer_Get_Data_Count() >= ADC_BATCH_SIZE) ? 1 : 0;
//     return true;
// }

// bool EXT_ADC_Buffer_Pop(AdcPair_t *data)
// {
//     if (g_usb_ring_buffer.head == g_usb_ring_buffer.tail) // TODO non-atomic read! fix!
//     {
//         return false; // Empty
//     }

//     *data = g_usb_ring_buffer.buffer[g_usb_ring_buffer.tail];
//     __DMB(); // Memory Barrier
//     g_usb_ring_buffer.tail = (g_usb_ring_buffer.tail + 1) & (USB_BUFFER_ELEMENTS - 1);

//     return true;
// }



// void ADC_ISR_Handler(void)
// {
//     /* 1. Clear EXTI Flag (PR1 register for G4) */
//     EXTI->PR1 |= EXTI_PR1_PIF4;

//     /* 2. Transaction 1: ADC1 */
//     /* CS Low */
//     CS_1_GPIO_Port->BSRR = (uint32_t)CS_1_Pin << 16U;

//     /* Configure DMA1 Ch2 (SPI1 RX) - Wait for RX Complete */
//     DMA1_Channel2->CMAR = (uint32_t)g_spi1_buf;
//     DMA1_Channel2->CNDTR = 3U;
//     DMA1_Channel2->CCR |= DMA_CCR_EN;

//     /* Configure DMA1 Ch3 (SPI1 TX) */
//     DMA1_Channel3->CMAR = (uint32_t)SPI_DUMMY_TX;
//     DMA1_Channel3->CNDTR = 3U;
//     DMA1_Channel3->CCR |= DMA_CCR_EN;

//     /* Enable SPI1 */
//     SPI1->CR1 |= SPI_CR1_SPE;

//     /* Wait for RX DMA Complete (TCIF2) */
//     while (!(DMA1->ISR & DMA_ISR_TCIF2))
//     {
//         __NOP();
//     }

//     /* Cleanup SPI1 */
//     DMA1->IFCR |= DMA_IFCR_CTCIF2; // Clear Flag

//     // TODO ensure we really need this:
//     //  // ✅ CLEAR all related flags
//     //  DMA1->IFCR |= DMA_IFCR_CTCIF2 | DMA_IFCR_CHTIF2 | DMA_IFCR_CTEIF2;
//     //  DMA1->IFCR |= DMA_IFCR_CTCIF3 | DMA_IFCR_CHTIF3 | DMA_IFCR_CTEIF3;

//     SPI1->CR1 &= ~SPI_CR1_SPE;         // Disable SPI
//     DMA1_Channel2->CCR &= ~DMA_CCR_EN; // Disable DMA
//     DMA1_Channel3->CCR &= ~DMA_CCR_EN;

//     /* CS High */
//     CS_1_GPIO_Port->BSRR = (uint32_t)CS_1_Pin;

//     /* 3. Transaction 2: ADC2 (Synchronized) */
//     /* Poll DRDY2 Pin with Timeout */
//     uint32_t timeout = DRDY2_TIMEOUT_CYCLES;
//     while ((DRDY_2_GPIO_Port->IDR & DRDY_2_Pin) && timeout--)
//     {
//         __NOP();
//     }

//     if (timeout > 0)
//     {
//         /* CS Low */
//         CS_2_GPIO_Port->BSRR = (uint32_t)CS_2_Pin << 16U;

//         /* Configure DMA2 Ch1 (SPI2 RX) - Wait for RX Complete */
//         DMA2_Channel1->CMAR = (uint32_t)g_spi2_buf;
//         DMA2_Channel1->CNDTR = 3U;
//         DMA2_Channel1->CCR |= DMA_CCR_EN;

//         /* Configure DMA2 Ch2 (SPI2 TX) */
//         DMA2_Channel2->CMAR = (uint32_t)SPI_DUMMY_TX;
//         DMA2_Channel2->CNDTR = 3U;
//         DMA2_Channel2->CCR |= DMA_CCR_EN;

//         /* Enable SPI2 */
//         SPI2->CR1 |= SPI_CR1_SPE;

//         /* Wait for RX DMA Complete (TCIF1) */
//         while (!(DMA2->ISR & DMA_ISR_TCIF1))
//         {
//             __NOP();
//         }

//         /* Cleanup SPI2 */
//         DMA2->IFCR |= DMA_IFCR_CTCIF1;
//         SPI2->CR1 &= ~SPI_CR1_SPE;
//         DMA2_Channel1->CCR &= ~DMA_CCR_EN;
//         DMA2_Channel2->CCR &= ~DMA_CCR_EN;

//         /* CS High */
//         CS_2_GPIO_Port->BSRR = (uint32_t)CS_2_Pin;

//         /* 4. Pack Data */
//         AdcPair_t packet;
//         packet.adc1[0] = g_spi1_buf[0];
//         packet.adc1[1] = g_spi1_buf[1];
//         packet.adc1[2] = g_spi1_buf[2];
//         packet.adc2[0] = g_spi2_buf[0];
//         packet.adc2[1] = g_spi2_buf[1];
//         packet.adc2[2] = g_spi2_buf[2];

//         if (!EXT_ADC_Buffer_Push(&packet))
//         {
//             g_adc_error_count++; // Overflow
//         }
//     }
//     else
//     {
//         /* Sync Loss Error */
//         g_adc_error_count++;
//     }
// }

// /* ==========================================================================
//    USB TRANSMISSION (Main Loop)
//    ========================================================================== */
// void USB_Process_Transmit(void)
// {
//     static AdcPair_t tx_batch[ADC_BATCH_SIZE];
//     static uint8_t usb_tx_buffer[ADC_BATCH_SIZE * ADC_PAIR_BYTES];

//     uint32_t available = USB_Buffer_Get_Data_Count();

//     if (available >= ADC_BATCH_SIZE)
//     {
//         /* Pop Batch */
//         for (int i = 0; i < ADC_BATCH_SIZE; i++)
//         {
//             EXT_ADC_Buffer_Pop(&tx_batch[i]);
//         }

//         /* Pack into Linear Buffer for USB */
//         for (int i = 0; i < ADC_BATCH_SIZE; i++)
//         {
//             uint32_t offset = i * ADC_PAIR_BYTES;
//             usb_tx_buffer[offset + 0] = tx_batch[i].adc1[0];
//             usb_tx_buffer[offset + 1] = tx_batch[i].adc1[1];
//             usb_tx_buffer[offset + 2] = tx_batch[i].adc1[2];
//             usb_tx_buffer[offset + 3] = tx_batch[i].adc2[0];
//             usb_tx_buffer[offset + 4] = tx_batch[i].adc2[1];
//             usb_tx_buffer[offset + 5] = tx_batch[i].adc2[2];
//         }

//         /* Send via CDC */
//         CDC_Transmit_FS(usb_tx_buffer, sizeof(usb_tx_buffer));
//     }
// }