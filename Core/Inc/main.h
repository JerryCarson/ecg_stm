/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.h
 * @brief          : Header for main.c file.
 *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C"
{
#endif

    /* Includes ------------------------------------------------------------------*/

#include "stm32g4xx_hal.h"

    /* Private includes ----------------------------------------------------------*/
    /* USER CODE BEGIN Includes */
#include "string.h"
    // #include "ring_buffer.h"

    /* USER CODE END Includes */

    /* Exported types ------------------------------------------------------------*/
    /* USER CODE BEGIN ET */

    /* USER CODE END ET */

    /* Exported constants --------------------------------------------------------*/
    /* USER CODE BEGIN EC */

    /* USER CODE END EC */

    /* Exported macro ------------------------------------------------------------*/
    /* USER CODE BEGIN EM */

    /* USER CODE END EM */

    /* Exported functions prototypes ---------------------------------------------*/
    void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define LO_M_Pin GPIO_PIN_1
#define LO_M_GPIO_Port GPIOF
#define LO_P_Pin GPIO_PIN_0
#define LO_P_GPIO_Port GPIOA
#define CS_2_Pin GPIO_PIN_6
#define CS_2_GPIO_Port GPIOC
#define RESET_2_Pin GPIO_PIN_8
#define RESET_2_GPIO_Port GPIOA
#define START_2_Pin GPIO_PIN_9
#define START_2_GPIO_Port GPIOA
#define DRDY_2_Pin GPIO_PIN_10
#define DRDY_2_GPIO_Port GPIOA
#define CS_1_Pin GPIO_PIN_10
#define CS_1_GPIO_Port GPIOC
#define RESET_1_Pin GPIO_PIN_11
#define RESET_1_GPIO_Port GPIOC
#define START_1_Pin GPIO_PIN_3
#define START_1_GPIO_Port GPIOB
#define DRDY_1_Pin GPIO_PIN_4
#define DRDY_1_GPIO_Port GPIOB

    /* USER CODE BEGIN Private defines */

    typedef enum
    {
        DATA_SPI_0,
        DATA_SPI_1,
        DATA_ADC_ECG,
        DATA_PC_CMD,
        DATA_PACKET_ERROR
    } StreamDataType;
    typedef enum
    {
        DATA_READY,
        DATA_WAIT
    } USBDataStatus;

    /*
    RINGBUFFER_DEFINE(uint8_t, RingBuffer_8, 256);
    RINGBUFFER_DEFINE(uint16_t, RingBuffer_16, 256);
    RINGBUFFER_DEFINE(uint32_t, RingBuffer_32, 1024);
    */

#define USB_BUF_SIZE 1024

#define SINE_WAVE_SAMPLES 100
#define DAC_RESOLUTION 4095.0 // 12-bit DAC

#define MAX_PACKET_SIZE 256
#define ADC_BUF_SIZE 32

    _Static_assert(MAX_PACKET_SIZE >= ADC_BUF_SIZE,
                   "Too large ADC_BUF_SIZE");

#define CMD_HEADER 0xAA

    extern uint8_t usb_rx_buf[USB_BUF_SIZE];

    extern USBDataStatus USB_Status;

    /* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
