#ifndef UTILITY_FUNCTIONS_H
#define UTILITY_FUNCTIONS_H

#include "main.h"
#include "adc_handler.h"
#include "ring_buffer.h"
#include "tim.h"

void DRDY_no_responce_timeout_handle(adc_dma_context_t *ctx);
void internal_DAC_EN_DIS_mgr();
void internal_ADC_EN_DIS_mgr();
StreamPacket_t create_packet(StreamDataType t, uint16_t len);


#endif