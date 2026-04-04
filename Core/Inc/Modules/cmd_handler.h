#ifndef CMD_HANDLER_H
#define CMD_HANDLER_H

#include "main.h"
#define CMD_TABLE_SIZE 10

typedef enum
{
    EXT_ADC_RST_CFG,
    STOP_ALL_ANALOG,
    EN_DAC,
    READ_ALL_CHANNELS,
    READ_I_CH_ONLY,
    READ_II_CH_ONLY,
    READ_ECG_ONLY,
    IGNORE_LO_DISRUPT,
    DISIGNORE_LO_DISRUPT,
    RESET_LATCHES
} CommandID;

/** @brief  Определение типа указателя на функцию, используется для создания таблицы команд.*/
typedef void (*cmd_handler_t)(Peripheral_latch_set *l);

typedef struct
{
    uint8_t cmd_id;
    cmd_handler_t handler;
} CommandEntry;

void stop_all(Peripheral_latch_set *l);
void reset_latches(Peripheral_latch_set *l);
void set_latches(Peripheral_latch_set *l);
void EXT_ADC_RST_RECONFIG(Peripheral_latch_set *l);
void enable_internal_DAC(Peripheral_latch_set *l);
void enable_both_external_ADC(Peripheral_latch_set *l);
void enable_external_ADC_I(Peripheral_latch_set *l);
void enable_external_ADC_II(Peripheral_latch_set *l);
void read_ecg_only(Peripheral_latch_set *l);
void ignore_LO_disrupt(Peripheral_latch_set *l);
void disignore_LO_disrupt(Peripheral_latch_set *l);

void process_command(uint8_t *payload, uint16_t len);

extern const CommandEntry cmd_table[CMD_TABLE_SIZE];

#endif