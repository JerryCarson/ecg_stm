/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    stm32g4xx_it.c
 * @brief   Interrupt Service Routines.
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2026 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32g4xx_it.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ring_buffer.h"
#include "adc_handler.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/
extern PCD_HandleTypeDef hpcd_USB_FS;
extern DMA_HandleTypeDef hdma_adc1;
extern DMA_HandleTypeDef hdma_dac1_ch1;
extern DMA_HandleTypeDef hdma_spi1_rx;
extern DMA_HandleTypeDef hdma_spi1_tx;
extern DMA_HandleTypeDef hdma_spi2_rx;
extern DMA_HandleTypeDef hdma_spi2_tx;
/* USER CODE BEGIN EV */

static volatile uint32_t adc1_batch_count = 0;
static volatile uint32_t adc2_batch_count = 0;
static inline bool adc_push(AdcRingBuffer_t *rb, uint8_t *data)
{
  uint32_t head = rb->head;
  uint32_t next = (head + 1) & (USB_BUFFER_ELEMENTS - 1);

  if (next == rb->tail)
    return false;

  rb->buffer[head].data[0] = data[0];
  rb->buffer[head].data[1] = data[1];
  rb->buffer[head].data[2] = data[2];

  __DMB();
  rb->head = next;

  return true;
}

static inline void ADC1_DRDY_ISR(void)
{
  /* Check if DMA still active */
  if (DMA1_Channel2->CCR & DMA_CCR_EN)
  {
    g_adc1_error_count++;
    return;
  }

  /* Clear DMA flags */
  DMA1->IFCR =
      DMA_IFCR_CTCIF2 | DMA_IFCR_CTEIF2 | DMA_IFCR_CHTIF2 |
      DMA_IFCR_CTCIF3 | DMA_IFCR_CTEIF3 | DMA_IFCR_CHTIF3;

  /* Optional: clear SPI state */
  if (SPI1->SR & SPI_SR_OVR)
  {
    (void)SPI1->DR;
    (void)SPI1->SR;
    g_adc1_error_count++;
  }

  /* CS LOW */
  __DMB();
  CS_1_GPIO_Port->BSRR = (uint32_t)CS_1_Pin << 16U;
  __DMB();

  /* Configure DMA */
  DMA1_Channel2->CMAR = (uint32_t)g_spi1_buf;
  DMA1_Channel2->CNDTR = 3;

  DMA1_Channel3->CMAR = (uint32_t)SPI_DUMMY_TX;
  DMA1_Channel3->CNDTR = 3;

  /* Enable RX first */
  DMA1_Channel2->CCR |= DMA_CCR_EN;
  __DMB();
  DMA1_Channel3->CCR |= DMA_CCR_EN;
}

static inline void ADC2_DRDY_ISR(void)
{
  if (DMA2_Channel1->CCR & DMA_CCR_EN)
  {
    g_adc2_error_count++;
    return;
  }

  DMA2->IFCR = DMA_IFCR_CTCIF1 | DMA_IFCR_CTEIF1 | DMA_IFCR_CHTIF1 |
               DMA_IFCR_CTCIF2 | DMA_IFCR_CTEIF2 | DMA_IFCR_CHTIF2;

  if (SPI2->SR & SPI_SR_OVR)
  {
    (void)SPI2->DR;
    (void)SPI2->SR;
    g_adc2_error_count++;
  }

  __DMB();
  CS_2_GPIO_Port->BSRR = (uint32_t)CS_2_Pin << 16U;
  __DMB();

  DMA2_Channel1->CMAR = (uint32_t)g_spi2_buf;
  DMA2_Channel1->CNDTR = 3;

  DMA2_Channel2->CMAR = (uint32_t)SPI_DUMMY_TX;
  DMA2_Channel2->CNDTR = 3;

  DMA2_Channel1->CCR |= DMA_CCR_EN;
  __DMB();
  DMA2_Channel2->CCR |= DMA_CCR_EN;
}

static inline void adc1_sample_ready(void)
{
  if (!adc_push(&adc1_buf, (uint8_t *)g_spi1_buf))
    g_adc1_error_count++;
}

static inline void adc2_sample_ready(void)
{
  if (!adc_push(&adc2_buf, (uint8_t *)g_spi2_buf))
    g_adc2_error_count++;
}
/* USER CODE END EV */

/******************************************************************************/
/*           Cortex-M4 Processor Interruption and Exception Handlers          */
/******************************************************************************/
/**
 * @brief This function handles Non maskable interrupt.
 */
void NMI_Handler(void)
{
  /* USER CODE BEGIN NonMaskableInt_IRQn 0 */

  /* USER CODE END NonMaskableInt_IRQn 0 */
  /* USER CODE BEGIN NonMaskableInt_IRQn 1 */
  while (1)
  {
  }
  /* USER CODE END NonMaskableInt_IRQn 1 */
}

/**
 * @brief This function handles Hard fault interrupt.
 */
void HardFault_Handler(void)
{
  /* USER CODE BEGIN HardFault_IRQn 0 */

  /* USER CODE END HardFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_HardFault_IRQn 0 */
    /* USER CODE END W1_HardFault_IRQn 0 */
  }
}

/**
 * @brief This function handles Memory management fault.
 */
void MemManage_Handler(void)
{
  /* USER CODE BEGIN MemoryManagement_IRQn 0 */

  /* USER CODE END MemoryManagement_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_MemoryManagement_IRQn 0 */
    /* USER CODE END W1_MemoryManagement_IRQn 0 */
  }
}

/**
 * @brief This function handles Prefetch fault, memory access fault.
 */
void BusFault_Handler(void)
{
  /* USER CODE BEGIN BusFault_IRQn 0 */

  /* USER CODE END BusFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_BusFault_IRQn 0 */
    /* USER CODE END W1_BusFault_IRQn 0 */
  }
}

/**
 * @brief This function handles Undefined instruction or illegal state.
 */
void UsageFault_Handler(void)
{
  /* USER CODE BEGIN UsageFault_IRQn 0 */

  /* USER CODE END UsageFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_UsageFault_IRQn 0 */
    /* USER CODE END W1_UsageFault_IRQn 0 */
  }
}

/**
 * @brief This function handles System service call via SWI instruction.
 */
void SVC_Handler(void)
{
  /* USER CODE BEGIN SVCall_IRQn 0 */

  /* USER CODE END SVCall_IRQn 0 */
  /* USER CODE BEGIN SVCall_IRQn 1 */

  /* USER CODE END SVCall_IRQn 1 */
}

/**
 * @brief This function handles Debug monitor.
 */
void DebugMon_Handler(void)
{
  /* USER CODE BEGIN DebugMonitor_IRQn 0 */

  /* USER CODE END DebugMonitor_IRQn 0 */
  /* USER CODE BEGIN DebugMonitor_IRQn 1 */

  /* USER CODE END DebugMonitor_IRQn 1 */
}

/**
 * @brief This function handles Pendable request for system service.
 */
void PendSV_Handler(void)
{
  /* USER CODE BEGIN PendSV_IRQn 0 */

  /* USER CODE END PendSV_IRQn 0 */
  /* USER CODE BEGIN PendSV_IRQn 1 */

  /* USER CODE END PendSV_IRQn 1 */
}

/**
 * @brief This function handles System tick timer.
 */
void SysTick_Handler(void)
{
  /* USER CODE BEGIN SysTick_IRQn 0 */

  /* USER CODE END SysTick_IRQn 0 */
  HAL_IncTick();
  /* USER CODE BEGIN SysTick_IRQn 1 */

  /* USER CODE END SysTick_IRQn 1 */
}

/******************************************************************************/
/* STM32G4xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32g4xx.s).                    */
/******************************************************************************/

/**
 * @brief This function handles EXTI line0 interrupt.
 */
void EXTI0_IRQHandler(void)
{
  /* USER CODE BEGIN EXTI0_IRQn 0 */

  /* USER CODE END EXTI0_IRQn 0 */
  /* USER CODE BEGIN EXTI0_IRQn 1 */

  /* USER CODE END EXTI0_IRQn 1 */
}

/**
 * @brief This function handles EXTI line1 interrupt.
 */
void EXTI1_IRQHandler(void)
{
  /* USER CODE BEGIN EXTI1_IRQn 0 */

  /* USER CODE END EXTI1_IRQn 0 */
  /* USER CODE BEGIN EXTI1_IRQn 1 */

  /* USER CODE END EXTI1_IRQn 1 */
}

/**
 * @brief This function handles EXTI line4 interrupt.
 */
void EXTI4_IRQHandler(void)
{
  /* USER CODE BEGIN EXTI4_IRQn 0 */
  if (EXTI->PR1 & EXTI_PR1_PIF4)
  {
    EXTI->PR1 = EXTI_PR1_PIF4;
    __DSB();
    ADC1_DRDY_ISR();
  }
  /* USER CODE END EXTI4_IRQn 0 */
  /* USER CODE BEGIN EXTI4_IRQn 1 */

  /* USER CODE END EXTI4_IRQn 1 */
}

/**
 * @brief This function handles DMA1 channel1 global interrupt.
 */
void DMA1_Channel1_IRQHandler(void)
{
  /* USER CODE BEGIN DMA1_Channel1_IRQn 0 */

  /* USER CODE END DMA1_Channel1_IRQn 0 */
  /* USER CODE BEGIN DMA1_Channel1_IRQn 1 */

  /* USER CODE END DMA1_Channel1_IRQn 1 */
}

/**
 * @brief This function handles DMA1 channel2 global interrupt.
 */
void DMA1_Channel2_IRQHandler(void)
{
  /* USER CODE BEGIN DMA1_Channel2_IRQn 0 */
  uint32_t isr = DMA1->ISR;

  if (isr & DMA_ISR_TEIF2)
  {
    DMA1->IFCR = DMA_IFCR_CTEIF2;

    /* Force clean state */
    DMA1_Channel3->CCR &= ~DMA_CCR_EN;
    DMA1_Channel2->CCR &= ~DMA_CCR_EN;

    /* Optional: clear SPI in case of corruption */
    if (SPI1->SR & SPI_SR_OVR)
    {
      (void)SPI1->DR;
      (void)SPI1->SR;
    }

    g_adc1_error_count++;
    return;
  }

  if (isr & DMA_ISR_TCIF2)
  {
    DMA1->IFCR = DMA_IFCR_CTCIF2;

    /* Ensure SPI finished */
    while (SPI1->SR & SPI_SR_BSY)
      ;

    /* Disable DMA: TX first */
    DMA1_Channel3->CCR &= ~DMA_CCR_EN;
    DMA1_Channel2->CCR &= ~DMA_CCR_EN;

    /* CS HIGH */
    __DMB();
    CS_1_GPIO_Port->BSRR = CS_1_Pin;

    /* OVR protection */
    if (SPI1->SR & SPI_SR_OVR)
    {
      (void)SPI1->DR;
      (void)SPI1->SR;
      g_adc1_error_count++;
    }

    adc1_sample_ready();
    adc1_batch_count++;
    if (adc1_batch_count >= ADC_BATCH_SIZE)
    {
      adc1_batch_size_reached = true;
      adc1_batch_count = 0; // reset counter
    }
  }
  /* USER CODE END DMA1_Channel2_IRQn 0 */
  /* USER CODE BEGIN DMA1_Channel2_IRQn 1 */

  /* USER CODE END DMA1_Channel2_IRQn 1 */
}

/**
 * @brief This function handles DMA1 channel3 global interrupt.
 */
void DMA1_Channel3_IRQHandler(void)
{
  /* USER CODE BEGIN DMA1_Channel3_IRQn 0 */
  // TX DMA usually doesn’t need processing for 3-byte transfers,
  // but clear transfer complete flag if needed
  if (DMA1->ISR & DMA_ISR_TCIF3) // Channel3 TX
  {
    DMA1->IFCR = DMA_IFCR_CTCIF3; // clear flag
  }
  /* USER CODE END DMA1_Channel3_IRQn 0 */
  /* USER CODE BEGIN DMA1_Channel3_IRQn 1 */

  /* USER CODE END DMA1_Channel3_IRQn 1 */
}

/**
 * @brief This function handles DMA1 channel4 global interrupt.
 */
void DMA1_Channel4_IRQHandler(void)
{
  /* USER CODE BEGIN DMA1_Channel4_IRQn 0 */

  /* USER CODE END DMA1_Channel4_IRQn 0 */
  /* USER CODE BEGIN DMA1_Channel4_IRQn 1 */

  /* USER CODE END DMA1_Channel4_IRQn 1 */
}

/**
 * @brief This function handles USB low priority interrupt remap.
 */
void USB_LP_IRQHandler(void)
{
  /* USER CODE BEGIN USB_LP_IRQn 0 */

  /* USER CODE END USB_LP_IRQn 0 */
  /* USER CODE BEGIN USB_LP_IRQn 1 */

  /* USER CODE END USB_LP_IRQn 1 */
}

/**
 * @brief This function handles EXTI line[15:10] interrupts.
 */
void EXTI15_10_IRQHandler(void)
{
  /* USER CODE BEGIN EXTI15_10_IRQn 0 */
  if (EXTI->PR1 & EXTI_PR1_PIF10)
  {
    EXTI->PR1 = EXTI_PR1_PIF10;
    __DSB();
    ADC2_DRDY_ISR();
  }
  /* USER CODE END EXTI15_10_IRQn 0 */
  /* USER CODE BEGIN EXTI15_10_IRQn 1 */

  /* USER CODE END EXTI15_10_IRQn 1 */
}

/**
 * @brief This function handles DMA2 channel1 global interrupt.
 */
void DMA2_Channel1_IRQHandler(void)
{
  /* USER CODE BEGIN DMA2_Channel1_IRQn 0 */
  uint32_t isr = DMA2->ISR;

  /* --- Transfer Error (must be handled first) --- */
  if (isr & DMA_ISR_TEIF1)
  {
    DMA2->IFCR = DMA_IFCR_CTEIF1;

    /* Force clean state */
    DMA2_Channel2->CCR &= ~DMA_CCR_EN;
    DMA2_Channel1->CCR &= ~DMA_CCR_EN;

    /* Optional: clear SPI in case of corruption */
    if (SPI2->SR & SPI_SR_OVR)
    {
      (void)SPI2->DR;
      (void)SPI2->SR;
    }

    g_adc2_error_count++;
    return;
  }

  /* --- Transfer Complete --- */
  if (isr & DMA_ISR_TCIF1)
  {
    DMA2->IFCR = DMA_IFCR_CTCIF1;

    /* Ensure last bit fully shifted out */
    while (SPI2->SR & SPI_SR_BSY)
      ;

    /* Disable DMA: TX first, then RX */
    DMA2_Channel2->CCR &= ~DMA_CCR_EN;
    DMA2_Channel1->CCR &= ~DMA_CCR_EN;

    /* CS HIGH */
    __DMB();
    CS_2_GPIO_Port->BSRR = CS_2_Pin;

    /* Overrun protection (rare but critical) */
    if (SPI2->SR & SPI_SR_OVR)
    {
      (void)SPI2->DR;
      (void)SPI2->SR;
      g_adc2_error_count++;
    }

    /* Push sample */
    adc2_sample_ready();
    adc2_batch_count++;
    if (adc2_batch_count >= ADC_BATCH_SIZE)
    {
      adc2_batch_size_reached = true;
      adc2_batch_count = 0;
    }
  }
  /* USER CODE END DMA2_Channel1_IRQn 0 */
  /* USER CODE BEGIN DMA2_Channel1_IRQn 1 */

  /* USER CODE END DMA2_Channel1_IRQn 1 */
}

/**
 * @brief This function handles DMA2 channel2 global interrupt.
 */
void DMA2_Channel2_IRQHandler(void)
{
  /* USER CODE BEGIN DMA2_Channel2_IRQn 0 */

  /* USER CODE END DMA2_Channel2_IRQn 0 */
  /* USER CODE BEGIN DMA2_Channel2_IRQn 1 */

  /* USER CODE END DMA2_Channel2_IRQn 1 */
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
