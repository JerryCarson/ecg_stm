/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
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
#include "adc.h"
#include "crc.h"
#include "dac.h"
#include "dma.h"
#include "spi.h"
#include "tim.h"
#include "usb_device.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "uplink_buffer.h"
#include "downlink_parser.h"
#include "adc_handler.h"
#include "cmd_handler.h"
#include "utility_functions.h"
// #include <stdint.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

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
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

ADC_Telemetry adc_telemetry = {0};

/** @brief Массив данных для генерации синуса (DAC1) */
static __ALIGNED(4) uint16_t sine_wave[SINE_WAVE_SAMPLES];

/** @brief Буфер значений внутреннего ADC (сигнал ЭКГ) */
static __ALIGNED(4) uint16_t ecg_buffer[ECG_BUF_SIZE];

/** @brief Кольцевой буфер для потока данных с ПК */
Downlink_USB_Stream usbStream;

uint8_t SPI_Request[] = {0xAA, 0xBB, 0xCC};
uint8_t SPI_Answer[3];

bool dac_running;
bool adc_running;

Peripheral_latch_set Latches;

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{

  /* USER CODE BEGIN 1 */
  usbStream.head = usbStream.tail = 0U;
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init(); 

  /* USER CODE BEGIN Init */
  GenerateSineWave(sine_wave);
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
  set_latches();
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_DAC1_Init();
  MX_SPI1_Init();
  MX_TIM6_Init();
  MX_ADC1_Init();
  MX_USB_Device_Init();
  MX_TIM7_Init();
  MX_SPI2_Init();
  MX_CRC_Init();
  /* USER CODE BEGIN 2 */

  // reset_latches(&Latches);

  // HAL_NVIC_DisableIRQ(DMA1_Channel3_IRQn); // SPI1 TX - not used
  // HAL_NVIC_DisableIRQ(DMA2_Channel2_IRQn); // SPI2 TX - not used
  // NVIC_DisableIRQ(DMA2_Channel2_IRQn);

  ADC_Handler_Init();
  stop_all();
  HAL_Delay(100);
  // ADC_setup(&adc2_ctx);
  // Latches.EXTERNAL_ADC_I_IsLocked = 0;
  // Latches.EXTERNAL_ADC_II_IsLocked = 0;
  // Latches.INTERNAL_ADC_IsLocked = 0;
  // Latches.INTERNAL_DAC_IsLocked = 1;

  ADC_setup(&adc1_ctx);
  ADC_setup(&adc2_ctx);

    /* Старт ADC1 (чтение сигнала ЭКГ по ивенту от TIM6) */
  if (HAL_ADC_Start_DMA(&hadc1, (uint32_t *)(void *)ecg_buffer, ECG_BUF_SIZE) != HAL_OK)
  {
    Error_Handler();
  }

  // HAL_ADC_Start_DMA(&hadc1, (uint32_t *)(void *)ecg_buffer, ECG_BUF_SIZE);

  /* Старт DAC1 (генерация синуса, отсчеты по таймеру TIM6) */
  // HAL_DAC_Start_DMA(&hdac1, DAC1_CHANNEL_1, (uint32_t *)(void *)sine_wave, SINE_WAVE_SAMPLES, DAC_ALIGN_12B_R);
  if (HAL_DAC_Start_DMA(&hdac1, DAC1_CHANNEL_1, (uint32_t *)(void *)sine_wave, SINE_WAVE_SAMPLES, DAC_ALIGN_12B_R) != HAL_OK)
  {
    Error_Handler();
  }

  dac_running = (bool)false;
  adc_running = (bool)false;
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    parse_downlink_data(&usbStream);
    DRDY_no_responce_timeout_handle(&adc1_ctx);
    DRDY_no_responce_timeout_handle(&adc2_ctx);
    processAdcBatches(&adc1_ctx);
    processAdcBatches(&adc2_ctx);
    stream_data_uplink(&EXT_ADC1_Stream);
    stream_data_uplink(&EXT_ADC2_Stream);
    stream_data_uplink(&INT_ADC_Stream);
    internal_DAC_EN_DIS_mgr();
    internal_ADC_EN_DIS_mgr();
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
   */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST); //-V2547

  /** Initializes the RCC Oscillators according to the specified parameters
   * in the RCC_OscInitTypeDef structure.
   */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI | RCC_OSCILLATORTYPE_HSI48;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV4;
  RCC_OscInitStruct.PLL.PLLN = 85; //-V2568
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/** @brief Двойная буферизация сигнала ЭКГ. Срабатывает  при заполнении первой половины буфера*/
void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc)
{
  if ((hadc->Instance == ADC1)) /* && (!Latches.INTERNAL_ADC_IsLocked) && ((!Latches.LO_DISRUPTED) && (!Latches.LO_SIGLNAL_USAGE_IsLocked))) */
  {
    if (Latches.LO_DISRUPTED)
    {
      if (!Latches.LO_SIGLNAL_USAGE_IsLocked)
      {
        return;
      }
    }
    // TODO Перепроверить, возможно схлопнуть в один колбэк

    StreamPacket_t packet = create_packet(DATA_ADC_ECG, (uint16_t)ECG_BUF_SIZE);

    for (uint16_t i = 0; i < ECG_BUF_SIZE / 2U; i++)
    {
      packet.data[i * 2U] = (uint8_t)(ecg_buffer[i] & 0xFFU);
      packet.data[i * 2U + 1U] = (uint8_t)((ecg_buffer[i] >> 8U) & 0xFFU);
    }

    pushPacket(&INT_ADC_Stream, &packet);
  }
}

/** @brief Двойная буферизация сигнала ЭКГ. Срабатывает  при заполнении второй половины буфера*/
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
  if ((hadc->Instance == ADC1)) /*&& (!Latches.INTERNAL_ADC_IsLocked) && ((!Latches.LO_DISRUPTED) && (!Latches.LO_SIGLNAL_USAGE_IsLocked)))*/
  {
    if (Latches.LO_DISRUPTED)
    {
      if (!Latches.LO_SIGLNAL_USAGE_IsLocked)
      {
        return;
      }
    }
    StreamPacket_t packet = create_packet(DATA_ADC_ECG, (uint16_t)ECG_BUF_SIZE);

    for (uint16_t i = 0U; i < ECG_BUF_SIZE / 2U; i++)
    {
      packet.data[i * 2U] = (uint8_t)(ecg_buffer[i + ECG_BUF_SIZE / 2U] & 0xFFU);
      packet.data[i * 2U + 1U] = (uint8_t)((ecg_buffer[i + ECG_BUF_SIZE / 2U] >> 8U) & 0xFFU);
    }

    pushPacket(&INT_ADC_Stream, &packet);
  }
}

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
