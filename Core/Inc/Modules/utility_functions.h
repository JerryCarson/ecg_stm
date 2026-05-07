#ifndef UTILITY_FUNCTIONS_H
#define UTILITY_FUNCTIONS_H

#include "main.h"
#include "adc_handler.h"
#include "uplink_buffer.h"
#include "tim.h"
#include <math.h>

void DRDY_no_responce_timeout_handle(adc_dma_context_t *ctx);
void internal_DAC_EN_DIS_mgr();
void internal_ADC_EN_DIS_mgr();
StreamPacket_t create_packet(StreamDataType t, uint16_t len);
void processAdcBatches(adc_dma_context_t *ctx);

/** @brief Функция для генерации значений синусоидального сигнала перед стартом основного цикла программы */
void GenerateSineWave(uint16_t *array);

#endif