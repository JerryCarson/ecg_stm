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
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32g4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "string.h"
#include <stdbool.h>
#include "compiler_defs.h"
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
    void start_device(GPIO_TypeDef *cs_port, uint16_t cs_pin, uint8_t src);
// void start_dev1();
// void start_dev2();
/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define LO_M_Pin GPIO_PIN_1
#define LO_M_GPIO_Port GPIOF
#define LO_M_EXTI_IRQn EXTI1_IRQn
#define LO_P_Pin GPIO_PIN_0
#define LO_P_GPIO_Port GPIOA
#define LO_P_EXTI_IRQn EXTI0_IRQn
#define CS_2_Pin GPIO_PIN_6
#define CS_2_GPIO_Port GPIOC
#define RESET_2_Pin GPIO_PIN_8
#define RESET_2_GPIO_Port GPIOA
#define START_2_Pin GPIO_PIN_9
#define START_2_GPIO_Port GPIOA
#define DRDY_1_Pin GPIO_PIN_10
#define DRDY_1_GPIO_Port GPIOC
#define DRDY_1_EXTI_IRQn EXTI15_10_IRQn
#define CS_1_Pin GPIO_PIN_11
#define CS_1_GPIO_Port GPIOC
#define RESET_1_Pin GPIO_PIN_3
#define RESET_1_GPIO_Port GPIOB
#define START_1_Pin GPIO_PIN_4
#define START_1_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

    typedef struct Peripheral_latch_set
    {
        volatile bool INTERNAL_DAC_LOCK;
        volatile bool INTERNAL_ADC_LOCK;
        volatile bool EXTERNAL_ADC_I_LOCK;
        volatile bool EXTERNAL_ADC_II_LOCK;
        volatile bool LO_DISRUPTED;
        volatile bool LO_SIGLNAL_USAGE_LOCK;

    } Peripheral_latch_set;

    extern Peripheral_latch_set Latches;

    /**
     * @brief Типы данных, отправляемых на ПК.
     * Тип данных записывается во второй байт пакета,
     * при желании каждому типу данных можно присвоить
     * пользовательское значение.
     * Список можно редактировать.
     * */
    typedef enum StreamDataType
    {
        DATA_NULL,
        DATA_SPI_1,        /**< Данные от первого внешнего ADC */
        DATA_SPI_2,        /**< Данные от второго внешнего ADC */
        DATA_ADC_ECG,      /**< Отсчеты сигнала ЭКГ с внутреннего ADC */
        DATA_PC_CMD,       /**< Команда контроллеру от ПК. Сама команда содержится в сегменте данных в пакете */
        DATA_PACKET_ERROR, /**< Команда от контроллера к ПК, сигнализирующая об ошибке сравнения контрольных сумм (пакет поврежден) */
        DATA_TM_I_ADC,
        DATA_TM_II_ADC
    } StreamDataType;

    typedef enum SPI_Source
    {
        SPI_SOURCE_ADC0,
        SPI_SOURCE_ADC1,
        SPI_SOURCE_BLANK
    } SPI_Source;

#define ADC_TM_REGS 10U

    typedef struct ADC_Telemetry
    {
        uint8_t adc1_reg_data[ADC_TM_REGS];
        uint8_t adc2_reg_data[ADC_TM_REGS];
    } ADC_Telemetry;

    /*
    RINGBUFFER_DEFINE(uint8_t, RingBuffer_8, 256);
    RINGBUFFER_DEFINE(uint16_t, RingBuffer_16, 256);
    RINGBUFFER_DEFINE(uint32_t, RingBuffer_32, 1024);
    */

#define SPI_PACKET_LEN 3

#define SINE_WAVE_SAMPLES 50
#define DAC_RESOLUTION 4095.0 // 12-bit DAC

#define MAX_PACKET_SIZE 128 /** Задает максимальный размер пакета данных в элементе \ref StreamPacket_t */

#define ECG_BUF_SIZE 128 /** Задает размер DMA буфера для внутреннего АЦП */

    _Static_assert(MAX_PACKET_SIZE >= ECG_BUF_SIZE / 2,
                   "Too large ECG_BUF_SIZE");

#define CMD_HEADER 0xAA                               /** Заголовочный байт пакетов */
#define HEADER_SIZE 4U                                /**< Размер заголовка пакета: [SYNC][TYPE][LEN_H][LEN_L] [байт]. */
#define CRC_SIZE 1U                                   /**< Размер контрольной суммы CRC-8 [байт]. */
#define MIN_PACKET_SIZE (HEADER_SIZE + CRC_SIZE + 1U) /**< Минимальный валидный размер пакета [байт]. */

    extern uint8_t SPI_Request[];
    extern uint8_t SPI_Answer[3];
    extern ADC_Telemetry adc_telemetry;

    extern bool dac_running;
    extern bool adc_running;

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
