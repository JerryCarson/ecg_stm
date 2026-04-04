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
#include <math.h>
#include "ring_buffer.h"
#include "usb_parser.h"
#include "adc_handler.h"
#include "cmd_handler.h"
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
void GenerateSineWave(void);
void processAdcBatches(void);
// void init_buffers(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

uint8_t usb_rx_buf[USB_BUF_SIZE];

/** @brief Массив данных для генерации синуса (DAC1) */
uint16_t sine_wave[SINE_WAVE_SAMPLES];

/** @brief Буфер значений ADC1 (сигнал ЭКГ) */
uint16_t adc_buffer[ADC_BUF_SIZE];

/** @brief Кольцевой буфер для потока данных с ПК */
USBStream usbStream;

uint8_t SPI_Request[] = {0x00, 0x00, 0x00};
uint8_t SPI_Answer[3];
StreamDataType source = DATA_NULL;
volatile uint16_t active_cs_pin;
GPIO_TypeDef *active_cs_port;
volatile uint8_t spi_busy = 0;

bool dac_running;
bool adc_running;

Peripheral_latch_set Latches;

// /** @brief Буфер для ЭКГ-сигнала с ADC1 */
// RingBuffer_16 ADC_ECG_BUF;

// /** @brief Буфер для приема команд с ПК */
// RingBuffer_8 PC_RX_BUF;

// /** @brief Буфер для приема команд с ПК */
// RingBuffer_8 PC_TX_BUF;

// /** @brief Буфер для записи сэмплов с внешних ADC */
// RingBuffer_32 SAMPLES_BUF;

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
  set_latches(&Latches);
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_DAC1_Init();
  MX_SPI1_Init();
  MX_TIM6_Init();
  MX_TIM7_Init();
  MX_ADC1_Init();
  // MX_USB_Device_Init();

  MX_SPI2_Init();
  MX_CRC_Init();
  /* USER CODE BEGIN 2 */
  // reset_latches(&Latches);

  HAL_NVIC_DisableIRQ(DMA1_Channel3_IRQn); // SPI1 TX - not used
  HAL_NVIC_DisableIRQ(DMA2_Channel2_IRQn); // SPI2 TX - not used

  ADC_Handler_Init();

  ADC_setup(&adc1_ctx);
  ADC_setup(&adc2_ctx); // TODO дописать управление пинами START

  read_ecg_only(&Latches);

  usbStream.head = usbStream.tail = 0;

  // /* Инициализация буферов: установка head и tail в 0 */
  // init_buffers();

  /* Старт таймера TIM6 для DAC (генерация синуса) и TIM7 для ADC (чтение сигнала ЭКГ)  */
  HAL_TIM_Base_Start(&htim6);
  HAL_TIM_Base_Start(&htim7);

  /* Старт ADC1 (чтение сигнала ЭКГ по ивенту от TIM6) */ // TODO перепроверить таймеры
  HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc_buffer, ADC_BUF_SIZE);

  /* Старт DAC1 (генерация синуса, отсчеты по таймеру TIM6) */
  HAL_DAC_Start_DMA(&hdac1, DAC1_CHANNEL_1, (uint32_t *)sine_wave, SINE_WAVE_SAMPLES, DAC_ALIGN_12B_R);
  dac_running = true;
  adc_running = true;

  // volatile uint8_t debug_gate = 1; // Change to 0 in VSCode Watch window to run
  // while (debug_gate)
  // {
  //   __NOP();
  // }
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    parser_process(&usbStream);
    processAdcBatches();
    USB_stream_data();
    if (Latches.INTERNAL_DAC_LOCK)
    {
      if (dac_running)
      {
        HAL_TIM_Base_Stop(&htim6);
        dac_running = false;
      }
    }
    else
    {
      if (!dac_running)
      {
        HAL_TIM_Base_Start(&htim6);
        dac_running = true;
      }
    }

    if (Latches.INTERNAL_ADC_LOCK)
    {
      if (adc_running)
      {
        HAL_TIM_Base_Stop(&htim7);
        adc_running = false;
      }
    }
    else
    {
      if (!adc_running)
      {
        HAL_TIM_Base_Start(&htim7);
        adc_running = true;
      }
    }
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
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
   * in the RCC_OscInitTypeDef structure.
   */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI | RCC_OSCILLATORTYPE_HSI48;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV2;
  RCC_OscInitStruct.PLL.PLLN = 25;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/** @brief Функция для генерации значений синусоидального сигнала перед стартом основного цикла программы */
void GenerateSineWave(void)
{
  for (uint16_t i = 0; i < SINE_WAVE_SAMPLES; i++)
  {
    double angle = (2.0 * 3.1415 * i) / SINE_WAVE_SAMPLES;

    double sine = sin(angle);

    // shift from [-1,1] to [0,1]
    sine = (sine + 1.0) / 2.0;

    // scale to DAC range
    sine_wave[i] = (uint16_t)(sine * DAC_RESOLUTION);
  }
}

/** @brief Двойная буферизация сигнала ЭКГ. Срабатывает  при заполнении первой половины буфера*/
void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc)
{
  if ((hadc->Instance == ADC1) && (!Latches.INTERNAL_ADC_LOCK) && ((!Latches.LO_DISRUPTED) && (!Latches.LO_SIGLNAL_USAGE_LOCK)))
  {
    // TODO Перепроверить, возможно схлопнуть в один колбэк
    StreamPacket_t packet;
    packet.dataType = DATA_ADC_ECG;
    packet.length = ADC_BUF_SIZE;

    for (int i = 0; i < ADC_BUF_SIZE / 2; i++)
    {
      packet.data[i * 2] = adc_buffer[i] & 0xFF;
      packet.data[i * 2 + 1] = (adc_buffer[i] >> 8) & 0xFF;
    }

    pushPacket(&packet);
  }
}

/** @brief Двойная буферизация сигнала ЭКГ. Срабатывает  при заполнении второй половины буфера*/
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
  if ((hadc->Instance == ADC1) && (!Latches.INTERNAL_ADC_LOCK) && ((!Latches.LO_DISRUPTED) && (!Latches.LO_SIGLNAL_USAGE_LOCK)))
  {
    // TODO Перепроверить
    StreamPacket_t packet;
    packet.dataType = DATA_ADC_ECG;
    packet.length = ADC_BUF_SIZE;

    for (int i = 0; i < ADC_BUF_SIZE / 2; i++)
    {
      packet.data[i * 2] = adc_buffer[i + ADC_BUF_SIZE / 2] & 0xFF;
      packet.data[i * 2 + 1] = (adc_buffer[i + ADC_BUF_SIZE / 2] >> 8) & 0xFF;
    }

    pushPacket(&packet);
  }
}

void processAdcBatches(void)
{
  StreamPacket_t packet;

  if (adc1_batch_size_reached)
  {
    adc1_batch_size_reached = false;
    __DMB();

    packet.dataType = DATA_SPI_1;
    packet.length = ADC_BATCH_SIZE * ADC_BYTES_PER_SAMPLE;
    for (uint32_t i = 0; i < ADC_BATCH_SIZE; i++)
    {
      uint32_t idx = (adc1_buf.tail + i) & (ADC_BUFFER_ELEMENTS - 1);
      memcpy(&packet.data[i * ADC_BYTES_PER_SAMPLE], adc1_buf.buffer[idx].data, ADC_BYTES_PER_SAMPLE);
    }
    // advance tail
    adc1_buf.tail = (adc1_buf.tail + ADC_BATCH_SIZE) & (ADC_BUFFER_ELEMENTS - 1);

    pushPacket(&packet);
  }

  if (adc2_batch_size_reached)
  {
    adc2_batch_size_reached = false;
    __DMB();

    packet.dataType = DATA_SPI_2;
    packet.length = ADC_BATCH_SIZE * ADC_BYTES_PER_SAMPLE;
    for (uint32_t i = 0; i < ADC_BATCH_SIZE; i++)
    {
      uint32_t idx = (adc2_buf.tail + i) & (ADC_BUFFER_ELEMENTS - 1);
      memcpy(&packet.data[i * ADC_BYTES_PER_SAMPLE], adc2_buf.buffer[idx].data, ADC_BYTES_PER_SAMPLE);
    }
    adc2_buf.tail = (adc2_buf.tail + ADC_BATCH_SIZE) & (ADC_BUFFER_ELEMENTS - 1);

    pushPacket(&packet);
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
