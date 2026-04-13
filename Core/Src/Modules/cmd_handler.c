#include "cmd_handler.h"
#include "adc_handler.h"

const CommandEntry cmd_table[CMD_TABLE_SIZE] = { //TODO дописать комадну для переконфигурирования ADC
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
    {TEST_SEND_SPI_DATA, test_send_spi_data}
};

void reset_latches(Peripheral_latch_set *l)
{
    l->EXTERNAL_ADC_I_LOCK = 0;
    l->EXTERNAL_ADC_II_LOCK = 0;
    l->INTERNAL_ADC_LOCK = 0;
    l->INTERNAL_DAC_LOCK = 0;
    l->LO_SIGLNAL_USAGE_LOCK = 0;
    l->LO_DISRUPTED = 0;
}

void set_latches(Peripheral_latch_set *l)
{
    l->EXTERNAL_ADC_I_LOCK = 1;
    l->EXTERNAL_ADC_II_LOCK = 1;
    l->INTERNAL_ADC_LOCK = 1;
    l->INTERNAL_DAC_LOCK = 1;
    l->LO_SIGLNAL_USAGE_LOCK = 1;
    l->LO_DISRUPTED = 1;
}

void EXT_ADC_RST_RECONFIG(Peripheral_latch_set *l) {} //TODO доделать

void stop_all(Peripheral_latch_set *l) //TODO дописать управление пинами START
{
    l->EXTERNAL_ADC_I_LOCK = 1;
    l->EXTERNAL_ADC_II_LOCK = 1;
    l->INTERNAL_ADC_LOCK = 1;
    l->INTERNAL_DAC_LOCK = 1;
}

void enable_internal_DAC(Peripheral_latch_set *l)
{
    l->INTERNAL_DAC_LOCK = 0;
}

void enable_both_external_ADC(Peripheral_latch_set *l)
{
    l->EXTERNAL_ADC_I_LOCK = 0;
    l->EXTERNAL_ADC_II_LOCK = 0;
}

void enable_external_ADC_I(Peripheral_latch_set *l)
{
    l->EXTERNAL_ADC_I_LOCK = 1;
    l->EXTERNAL_ADC_II_LOCK = 0;
}

void enable_external_ADC_II(Peripheral_latch_set *l)
{
    l->EXTERNAL_ADC_I_LOCK = 0;
    l->EXTERNAL_ADC_II_LOCK = 1;
}

void read_ecg_only(Peripheral_latch_set *l)
{
    l->EXTERNAL_ADC_I_LOCK = 1;
    l->EXTERNAL_ADC_II_LOCK = 1;
    l->INTERNAL_ADC_LOCK = 0;
}

void ignore_LO_disrupt(Peripheral_latch_set *l)
{
    l->LO_SIGLNAL_USAGE_LOCK = 1;
}

void disignore_LO_disrupt(Peripheral_latch_set *l)
{
    l->LO_SIGLNAL_USAGE_LOCK = 0;
}

void test_send_spi_data(Peripheral_latch_set *l)
{
    uint8_t SPI_Request[3] = {0xAA, 0xBB, 0xCC};
    uint8_t SPI_Answer[3] = {0};

    adc_dma_context_t *ctx = &adc1_ctx; // Example: using ADC1 context for this test
    SPI_DMA_TX_RX_byte_array(ctx, SPI_Request, SPI_Answer, 3, false);
}


void process_command(uint8_t *payload, uint16_t len)
{
    if (len == 0)
        return;

    uint8_t cmd = payload[0];

    for (uint32_t i = 0; i < CMD_TABLE_SIZE; i++)
    {
        if (cmd_table[i].cmd_id == cmd)
        {
            cmd_table[i].handler(&Latches);
            return;
        }
    }

    // unknown command → optionally send error
}