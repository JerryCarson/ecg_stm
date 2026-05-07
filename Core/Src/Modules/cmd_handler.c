#include "cmd_handler.h"
#include "adc_handler.h"
#include "usbd_cdc_if.h"
#include "uplink_buffer.h"
#include "utility_functions.h"

const CommandEntry cmd_table[CMD_TABLE_SIZE] = {{RESET_LATCHES, reset_latches},
                                                {STOP_ALL_ANALOG, stop_all},
                                                {EXT_ADC_RST_CFG, EXT_ADC_RST_RECONFIG},
                                                {EN_DAC, enable_internal_DAC},
                                                {READ_ALL_CHANNELS, enable_both_external_ADC},
                                                {READ_I_CH_ONLY, enable_external_ADC_I},
                                                {READ_II_CH_ONLY, enable_external_ADC_II},
                                                {READ_ECG_ONLY, read_ecg_only},
                                                {IGNORE_LO_DISRUPT, ignore_LO_disrupt},
                                                {DISIGNORE_LO_DISRUPT, disignore_LO_disrupt},
                                                {TEST_SEND_SPI_DATA, test_send_spi_data},
                                                {READ_EXT_ADCs_REGS, read_ext_adc_regs}};

/**
 * @private
 * @brief Запрос байта из регистра АЦП через SPI.
 * @param[in] ctx  Контекст DMA-обмена.
 * @param[in] reg  Адрес регистра (0x0..0xF).
 * @retval uint8_t Значение прочитанного регистра.
 * @note Использует синхронный SPI-DMA обмен. Блокирует поток до завершения.
 */
uint8_t request_ADC_reg_data(adc_dma_context_t *ctx, uint8_t reg)
{
    uint8_t SPI_Request[2] = {0x40U + reg, 0x00U};
    uint8_t SPI_Answer[2] = {0U, 0U};
    SPI_DMA_TX_RX_byte_array(ctx, SPI_Request, SPI_Answer, 2, false);

    uint8_t SPI_Request1[2] = {0x00U, 0x00U};
    uint8_t SPI_Answer1[2] = {0U, 0U};
    SPI_DMA_TX_RX_byte_array(ctx, SPI_Request1, SPI_Answer1, 2, false);
    __DSB();
    return SPI_Answer1[0];
}

void read_ext_adc_regs(void)
{
    NVIC_DisableIRQ(EXTI4_IRQn);
    NVIC_DisableIRQ(EXTI15_10_IRQn);
    NVIC_DisableIRQ(DMA1_Channel2_IRQn);
    NVIC_DisableIRQ(DMA2_Channel1_IRQn);
    // const uint8_t regs_count = 9;
    for (size_t i = 4; i < 9; i++)
    {
        adc_telemetry.adc1_reg_data[i] = request_ADC_reg_data(&adc1_ctx, i);
        adc_telemetry.adc2_reg_data[i] = request_ADC_reg_data(&adc2_ctx, i);
    }
    __DSB();
    StreamPacket_t packet = create_packet(DATA_TM_I_ADC, ADC_TM_REGS);
    memcpy(&packet.data, adc_telemetry.adc1_reg_data, ADC_TM_REGS);
    pushPacket(&EXT_ADC1_Stream, &packet);

    StreamPacket_t packet1 = create_packet(DATA_TM_II_ADC, ADC_TM_REGS);
    memcpy(&packet1.data, adc_telemetry.adc2_reg_data, ADC_TM_REGS);
    pushPacket(&EXT_ADC2_Stream, &packet1);

    NVIC_EnableIRQ(EXTI4_IRQn);
    NVIC_EnableIRQ(EXTI15_10_IRQn);
    NVIC_EnableIRQ(DMA1_Channel2_IRQn);
    NVIC_EnableIRQ(DMA2_Channel1_IRQn);
}

void reset_latches(void)
{
    Latches.EXTERNAL_ADC_I_LOCK = 0;
    Latches.EXTERNAL_ADC_II_LOCK = 0;
    Latches.INTERNAL_ADC_LOCK = 0;
    Latches.INTERNAL_DAC_LOCK = 0;
    Latches.LO_SIGLNAL_USAGE_LOCK = 0;
    Latches.LO_DISRUPTED = 0;
}

void set_latches(void)
{
    Latches.EXTERNAL_ADC_I_LOCK = 1;
    Latches.EXTERNAL_ADC_II_LOCK = 1;
    Latches.INTERNAL_ADC_LOCK = 1;
    Latches.INTERNAL_DAC_LOCK = 1;
    Latches.LO_SIGLNAL_USAGE_LOCK = 1;
    Latches.LO_DISRUPTED = 1;
}

void EXT_ADC_RST_RECONFIG(void)
{
    ADC_setup(&adc1_ctx);
    ADC_setup(&adc2_ctx);
} // TODO Проверить работоспособность

void stop_all(void) // TODO дописать управление пинами START
{
    Latches.EXTERNAL_ADC_I_LOCK = 1;
    Latches.EXTERNAL_ADC_II_LOCK = 1;
    Latches.INTERNAL_ADC_LOCK = 1;
    Latches.INTERNAL_DAC_LOCK = 1;
    adc1_ctx.start_port->BSRR = (uint32_t)adc1_ctx.start_pin << 16U; // Pull START LOW
    adc2_ctx.start_port->BSRR = (uint32_t)adc2_ctx.start_pin << 16U; // Pull START LOW
}

void enable_internal_DAC(void)
{
    Latches.INTERNAL_DAC_LOCK = 0;
}

void enable_both_external_ADC(void)
{
    Latches.EXTERNAL_ADC_I_LOCK = 0;
    Latches.EXTERNAL_ADC_II_LOCK = 0;
    adc1_ctx.start_port->BSRR = adc1_ctx.start_pin;
    adc2_ctx.start_port->BSRR = adc2_ctx.start_pin;
}

void enable_external_ADC_I(void)
{
    Latches.EXTERNAL_ADC_I_LOCK = 0;
    Latches.EXTERNAL_ADC_II_LOCK = 1;
    adc1_ctx.start_port->BSRR = adc1_ctx.start_pin;
}

void enable_external_ADC_II(void)
{
    Latches.EXTERNAL_ADC_I_LOCK = 1;
    Latches.EXTERNAL_ADC_II_LOCK = 0;
    adc2_ctx.start_port->BSRR = adc2_ctx.start_pin;
}

void read_ecg_only(void)
{
    Latches.EXTERNAL_ADC_I_LOCK = 1;
    Latches.EXTERNAL_ADC_II_LOCK = 1;
    Latches.INTERNAL_ADC_LOCK = 0;
}

void ignore_LO_disrupt(void)
{
    Latches.LO_SIGLNAL_USAGE_LOCK = 1;
}

void disignore_LO_disrupt(void)
{
    Latches.LO_SIGLNAL_USAGE_LOCK = 0;
}

void test_send_spi_data(void)
{
    const uint8_t SPI_Request[3] = {0xAA, 0xBB, 0xCC};
    volatile uint8_t SPI_Answer[3] = {0};

    adc_dma_context_t *ctx = &adc2_ctx; // Example: using ADC1 context for this test
    SPI_DMA_TX_RX_byte_array(ctx, SPI_Request, SPI_Answer, 3, false);
}

void process_command(const uint8_t *payload, uint16_t len)
{
    if (len == 0)
        return;

    uint8_t cmd = payload[0];

    for (uint32_t i = 0; i < CMD_TABLE_SIZE; i++)
    {
        if (cmd_table[i].cmd_id == cmd)
        {
            cmd_table[i].handler();
            return;
        }
    }

    // unknown command → optionally send error
}