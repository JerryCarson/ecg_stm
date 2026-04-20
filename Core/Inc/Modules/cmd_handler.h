/**
 * @file cmd_handler.h
 * @brief Обработчик команд от ПК.
 */

#ifndef CMD_HANDLER_H
#define CMD_HANDLER_H

#include "main.h"

/**
 * @defgroup COMMAND_HANDLER Обработчик команд от ПК.
 * @{
 */

/**
 * @def CMD_TABLE_SIZE
 * @brief Размер таблицы команд (количество команд).
 */
#define CMD_TABLE_SIZE 11

/**
 * @brief Список доступных команд.
 */
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
    RESET_LATCHES,
    TEST_SEND_SPI_DATA
} CommandID;

/** @brief  Тип указателя на функцию-обработчик команды.
 * @param[in,out] l Указатель на структуру флагов периферии.
 */
typedef void (*cmd_handler_t)(Peripheral_latch_set *l);

/**
 * @brief Структура элемента таблицы команд.
 */
typedef struct CommandEntry
{
    uint8_t cmd_id;        /**< Идентификатор команды. Должен быть уникальным в рамках таблицы. */
    cmd_handler_t handler; /**< Указатель на обработчик. Вызывается при совпадении cmd_id. */
} CommandEntry;

/** @brief Остановить работу всей периферии. */
void stop_all(Peripheral_latch_set *l);

/** @brief Сбросить все @ref Latches в 0 (разблокировать работу всей периферии). */
void reset_latches(Peripheral_latch_set *l);

/** @brief Сбросить все @ref Latches в 1 (заблокировать работу всей периферии). */
void set_latches(Peripheral_latch_set *l);

/** @brief Сбросить и переконфигурировать внешние ADC */
void EXT_ADC_RST_RECONFIG(Peripheral_latch_set *l);

/** @brief Активировать внутренний DAC (генерация синуса) */
void enable_internal_DAC(Peripheral_latch_set *l);

/** @brief Активировать сбор данных с обоих внешних ADC */
void enable_both_external_ADC(Peripheral_latch_set *l);

/** @brief Активировать сбор данных с I внешнего ADC */
void enable_external_ADC_I(Peripheral_latch_set *l);

/** @brief Активировать сбор данных с II внешнего ADC */
void enable_external_ADC_II(Peripheral_latch_set *l);

/** @brief Сбор данных только с внутреннего ADC (экг сигнал) */
void read_ecg_only(Peripheral_latch_set *l);

/** @brief Игнорировать отрыв электрода */
void ignore_LO_disrupt(Peripheral_latch_set *l);

/** @brief Не игнорировать отрыв электрода */
void disignore_LO_disrupt(Peripheral_latch_set *l);

/** @brief Тест отправки данных по SPI */
void test_send_spi_data(Peripheral_latch_set *l);

/** @brief Обработчик команд от ПК. Ищет полученный от ПК id команды и исполняет команду
 * с таким id если он будет найден.
 */
void process_command(uint8_t *payload, uint16_t len);

/** @brief Таблица возможных команд */
extern const CommandEntry cmd_table[CMD_TABLE_SIZE];

/** @} */
#endif