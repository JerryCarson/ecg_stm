#include "cmd_handler.h"
#include "adc_handler.h"
#include "usbd_cdc_if.h"
#include "uplink_buffer.h"
#include "utility_functions.h"

const CommandEntry cmd_table[] = {
    {RESET_LATCHES, reset_latches},
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
    {EXT_ADC_TM_REQUEST, ext_adc_TM_request},
    {CONFIG_EXT_ADC_I, config_ext_adc_I},
    {CONFIG_EXT_ADC_II, config_ext_adc_II}
};

#define CONF_REGS_AMOUNT 13
#define FIRST_CONF_REG_ADDR 0x03U
uint8_t ext_adc_configs[CONF_REGS_AMOUNT] = {0};

static void disable_IRQs(adc_context *ctx)
{
    NVIC_DisableIRQ(ctx->drdy_irq);
    NVIC_DisableIRQ(ctx->rx_irq);
}

static void enable_IRQs(adc_context *ctx)
{
    NVIC_EnableIRQ(ctx->drdy_irq);
    NVIC_EnableIRQ(ctx->rx_irq);
}

/**
 * @private
 * @brief Запрос байта из регистра АЦП через SPI.
 * @param[in] ctx  Контекст DMA-обмена.
 * @param[in] reg  Адрес регистра (0x0..0xF).
 * @retval uint8_t Значение прочитанного регистра.
 * @note Использует синхронный SPI-DMA обмен. Блокирует поток до завершения.
 */
static uint8_t request_ADC_reg_data(adc_context *ctx, uint8_t reg)
{
    uint8_t SPI_Request_loc[2] = {(0x40U + reg), 0x00U};
    uint8_t SPI_Answer_loc[2] = {0U, 0U};
    SPI_DMA_TX_RX_byte_array(ctx, SPI_Request_loc, SPI_Answer_loc, 2, false);
    __DSB();
    __ISB();
    for (size_t j = 0U; j < 200U; j++)
    {
        __NOP();
    }
    __DSB();
    __ISB();
    uint8_t SPI_Request1[2] = {0x00U, 0x00U};
    uint8_t SPI_Answer1[2] = {0U, 0U};
    SPI_DMA_TX_RX_byte_array(ctx, SPI_Request1, SPI_Answer1, 2, false);
    __DSB();
    return SPI_Answer1[0];
}

void config_ext_adc_I()
{
    disable_IRQs(&adc1_ctx);
    adc1_ctx.rx->CCR &= ~DMA_CCR_EN;
    for (size_t i = 0; i < CONF_REGS_AMOUNT; i++)
    {
        if (ext_adc_configs[i] != 0xFF)
        {
            ADC_set_reg(&adc1_ctx, i + FIRST_CONF_REG_ADDR, ext_adc_configs[i]);
        }
    }
    enable_IRQs(&adc1_ctx);
}

void config_ext_adc_II()
{
    disable_IRQs(&adc2_ctx);
    adc2_ctx.tx->CCR &= ~DMA_CCR_EN;
    for (size_t i = 0; i < CONF_REGS_AMOUNT; i++)
    {
        if (ext_adc_configs[i] != 0xFF)
        {
            ADC_set_reg(&adc2_ctx, i + FIRST_CONF_REG_ADDR, ext_adc_configs[i]);
        }
    }
    enable_IRQs(&adc2_ctx);
}

void ext_adc_TM_request(void)
{
    disable_IRQs(&adc1_ctx);
    disable_IRQs(&adc2_ctx);
    // const uint8_t regs_count = 9;
    for (uint8_t i = 3U; i < ADC_TM_REGS; i++)
    {
        adc_telemetry.adc1_reg_data[i] = request_ADC_reg_data(&adc1_ctx, i);
        adc_telemetry.adc2_reg_data[i] = request_ADC_reg_data(&adc2_ctx, i);
    }
    __DSB();
    Uplink_Packet packet = create_packet(DATA_TM_I_ADC, (uint16_t)ADC_TM_REGS);
    (void)memcpy(&packet.data, adc_telemetry.adc1_reg_data, ADC_TM_REGS);
    pushPacket(&EXT_ADC1_Stream, &packet);

    packet = create_packet(DATA_TM_II_ADC, (uint16_t)ADC_TM_REGS);
    (void)memcpy(&packet.data, adc_telemetry.adc2_reg_data, ADC_TM_REGS);
    pushPacket(&EXT_ADC2_Stream, &packet);

    enable_IRQs(&adc1_ctx);
    enable_IRQs(&adc2_ctx);
}

void reset_latches(void)
{
    Latches.EXTERNAL_ADC_I_IsLocked = (bool)false;
    Latches.EXTERNAL_ADC_II_IsLocked = (bool)false;
    Latches.INTERNAL_ADC_IsLocked = (bool)false;
    Latches.INTERNAL_DAC_IsLocked = (bool)false;
    Latches.LO_SIGLNAL_USAGE_IsLocked = (bool)false;
    Latches.LO_DISRUPTED = (bool)false;
}

void set_latches(void)
{
    Latches.EXTERNAL_ADC_I_IsLocked = (bool)true;
    Latches.EXTERNAL_ADC_II_IsLocked = (bool)true;
    Latches.INTERNAL_ADC_IsLocked = (bool)true;
    Latches.INTERNAL_DAC_IsLocked = (bool)true;
    Latches.LO_SIGLNAL_USAGE_IsLocked = (bool)true;
    Latches.LO_DISRUPTED = (bool)true;
}

void EXT_ADC_RST_RECONFIG(void)
{
    ADC_setup(&adc1_ctx);
    ADC_setup(&adc2_ctx);
}

void stop_all(void)
{
    Latches.EXTERNAL_ADC_I_IsLocked = (bool)true;
    Latches.EXTERNAL_ADC_II_IsLocked = (bool)true;
    Latches.INTERNAL_ADC_IsLocked = (bool)true;
    Latches.INTERNAL_DAC_IsLocked = (bool)true;
    adc1_ctx.start_port->BSRR = (uint32_t)adc1_ctx.start_pin << 16U; // Pull START LOW
    adc2_ctx.start_port->BSRR = (uint32_t)adc2_ctx.start_pin << 16U; // Pull START LOW
}

void enable_internal_DAC(void)
{
    Latches.INTERNAL_DAC_IsLocked = (bool)false;
}

void enable_both_external_ADC(void)
{
    Latches.EXTERNAL_ADC_I_IsLocked = (bool)false;
    Latches.EXTERNAL_ADC_II_IsLocked = (bool)false;
    adc1_ctx.start_port->BSRR = adc1_ctx.start_pin;
    adc2_ctx.start_port->BSRR = adc2_ctx.start_pin;
}

void enable_external_ADC_I(void)
{
    Latches.EXTERNAL_ADC_I_IsLocked = (bool)false;
    Latches.EXTERNAL_ADC_II_IsLocked = (bool)true;
    adc1_ctx.start_port->BSRR = adc1_ctx.start_pin;
}

void enable_external_ADC_II(void)
{
    Latches.EXTERNAL_ADC_I_IsLocked = (bool)true;
    Latches.EXTERNAL_ADC_II_IsLocked = (bool)false;
    adc2_ctx.start_port->BSRR = adc2_ctx.start_pin;
}

void read_ecg_only(void)
{
    Latches.EXTERNAL_ADC_I_IsLocked = (bool)true;
    Latches.EXTERNAL_ADC_II_IsLocked = (bool)true;
    Latches.INTERNAL_ADC_IsLocked = (bool)false;
}

void ignore_LO_disrupt(void)
{
    Latches.LO_SIGLNAL_USAGE_IsLocked = (bool)true;
}

void disignore_LO_disrupt(void)
{
    Latches.LO_SIGLNAL_USAGE_IsLocked = (bool)false;
}

void test_send_spi_data(void)
{
    const uint8_t SPI_Request_loc[3] = {0xAAU, 0xBBU, 0xCCU};
    volatile uint8_t SPI_Answer_loc[3] = {0U, 0U, 0U};

    adc_context *ctx = &adc2_ctx; // Example: using ADC1 context for this test
    SPI_DMA_TX_RX_byte_array(ctx, SPI_Request_loc, SPI_Answer_loc, 3, false);
}

void process_command(const uint8_t *payload, uint16_t len) //-V2506
{
    if (len == 0U)
    {
        return;
    }

    uint8_t cmd = payload[0];
    if ((cmd == CONFIG_EXT_ADC_I) && (len == CONF_REGS_AMOUNT + 1))
    {
        memmove(ext_adc_configs, &payload[1], CONF_REGS_AMOUNT);
    }

    for (uint32_t i = 0U; i < sizeof(cmd_table) / sizeof(*cmd_table); i++)
    {
        if (cmd_table[i].cmd_id == cmd)
        {
            cmd_table[i].handler();
            return;
        }
    }

    // unknown command → optionally send error
}