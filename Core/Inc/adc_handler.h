#ifndef ADC_HANDLER_H
#define ADC_HANDLER_H

#include "stm32g4xx.h"
#include "usbd_cdc_if.h"
#include <stdint.h>
#include <stdbool.h>
#include "main.h"

/* Timeout for DRDY2 (Approx 2us at 170MHz) */
#define DRDY2_TIMEOUT_CYCLES 340U

// typedef struct
// {
//    uint8_t adc1[ADC_BYTES_PER_SAMPLE];
//    uint8_t adc2[ADC_BYTES_PER_SAMPLE];
// } AdcPair_t;

// typedef struct
// {
//    AdcPair_t buffer[ADC_BUFFER_ELEMENTS];
//    volatile uint32_t head;
//    volatile uint32_t tail;
// } UsbADCRingBuffer_t;

#define ADC_BYTES_PER_SAMPLE 3 // 24-bit
#define ADC_PAIR_BYTES (ADC_BYTES_PER_SAMPLE * 2U)
#define ADC_BATCH_SIZE 16       // Send 16 pairs per USB packet
#define ADC_BUFFER_ELEMENTS 256 // Ring buffer size (pairs)

_Static_assert((ADC_BUFFER_ELEMENTS & (ADC_BUFFER_ELEMENTS - 1)) == 0,
               "ADC_BUFFER_ELEMENTS must be power of two");

typedef struct
{
   uint8_t data[3];
} AdcSample_t;

typedef struct
{
   volatile AdcSample_t buffer[ADC_BUFFER_ELEMENTS];
   volatile uint32_t head;
   volatile uint32_t tail;
} AdcRingBuffer_t;

// extern UsbADCRingBuffer_t g_usb_ring_buffer;
extern volatile uint32_t g_adc_error_count;
extern volatile uint32_t g_adc1_error_count;
extern volatile uint32_t g_adc2_error_count;
extern volatile bool batch_size_reached;
extern volatile bool adc1_batch_size_reached;
extern volatile bool adc2_batch_size_reached;


extern volatile uint8_t g_spi1_buf[];
extern volatile uint8_t g_spi2_buf[];
extern const uint8_t SPI_DUMMY_TX[];

extern AdcRingBuffer_t adc1_buf;
extern AdcRingBuffer_t adc2_buf;

// void ADC_Handler_Init(void);
// void ADC_ISR_Handler(void); // Called from stm32g4xx_it.c
// bool EXT_ADC_Buffer_Push(AdcPair_t *data);
// bool EXT_ADC_Buffer_Pop();
uint32_t USB_Buffer_Get_Data_Count(void);
void ADC_Handler_Init(void);

#endif /* ADC_HANDLER_H */