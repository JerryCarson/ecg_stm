/**
 * @file cmd_handler.h
 * @brief Обработчик команд от ПК
 * @addtogroup COMMAND_HANDLER Обработчик команд от ПК
 * @{
 */

#ifndef CMD_HANDLER_H
#define CMD_HANDLER_H

#include "main.h"

/**
 * @def CMD_TABLE_SIZE
 * @brief Размер таблицы команд (количество поддерживаемых команд).
 */
#define CMD_TABLE_SIZE 12U

/**
 * @enum CommandID
 * @brief Идентификаторы доступных команд.
 * @note Значения должны совпадать с протоколом обмена с ПК.
 */
typedef enum CommandID
{
    EXT_ADC_RST_CFG,      /**< Перепрошивка внешних ADC */
    STOP_ALL_ANALOG,      /**< Остановка всех ADC и DAC */
    EN_DAC,               /**< Включение DAC (генерация синусоидального сигнала) */
    READ_ALL_CHANNELS,    /**< Чтение данных обоих внешних ADC */
    READ_I_CH_ONLY,       /**< Чтение данных только I внешнего ADC */
    READ_II_CH_ONLY,      /**< Чтение данных только II внешнего ADC */
    READ_ECG_ONLY,        /**< Чтение только сигнала ECG  */
    IGNORE_LO_DISRUPT,    /**< Игнорировать отрыв электродов */
    DISIGNORE_LO_DISRUPT, /**< Не игнорировать отрыв электродов */
    RESET_LATCHES,        /**< Включить всё (выставить все @ref Latches в 0) */
    TEST_SEND_SPI_DATA,   /**< Тестовая команда для отправки данных по SPI */
    READ_EXT_ADCs_REGS,    /**< Прочитать регистры обоих внешних ADC. Данные отправляются в @ref adc_telemetry */
} CommandID;

/**
 * @brief Тип указателя на функцию-обработчик команды.
 * @note Все обработчики должны иметь сигнатуру без параметров.
 */
typedef void (*cmd_handler_t)(void);

/** @brief Структура элемента таблицы команд.*/
typedef struct CommandEntry
{
    uint8_t cmd_id;        /**< Идентификатор команды. Должен быть уникальным в рамках таблицы. */
    cmd_handler_t handler; /**< Указатель на обработчик. Вызывается при совпадении cmd_id. */
} CommandEntry;

/**
 * @brief Чтение регистров внешнего ADC.
 * @attention Функция временно отключает прерывания EXTI4 и EXTI15_10.
 *            Блокирует поток данных до завершения чтения регистров.
 */
void read_ext_adc_regs(void);

/** @brief Остановить работу аналоговой периферии. */
void stop_all(void);

/** @brief Сбросить все @ref Latches в 0 (разблокировать работу всей периферии). */
void reset_latches(void);

/** @brief Сбросить все @ref Latches в 1 (заблокировать работу всей периферии). */
void set_latches(void);

/** @brief Сбросить и переконфигурировать внешние ADC */
void EXT_ADC_RST_RECONFIG(void);

/** @brief Активировать внутренний DAC (генерация синуса) */
void enable_internal_DAC(void);

/** @brief Активировать сбор данных с обоих внешних ADC */
void enable_both_external_ADC(void);

/** @brief Активировать сбор данных с I внешнего ADC */
void enable_external_ADC_I(void);

/** @brief Активировать сбор данных с II внешнего ADC */
void enable_external_ADC_II(void);

/** @brief Сбор данных только с внутреннего ADC (экг сигнал) */
void read_ecg_only(void);

/** @brief Игнорировать отрыв электрода */
void ignore_LO_disrupt(void);

/** @brief Не игнорировать отрыв электрода */
void disignore_LO_disrupt(void);

/** @brief Тест отправки данных по SPI */
void test_send_spi_data(void);

/** @brief Обработчик команд от ПК. Ищет полученный от ПК id команды и исполняет команду
 * с таким id если он будет найден.
 * @param[in] payload Указатель на буфер с данными от ПК.
 * @param[in] len     Длина буфера (в байтах).
 * @note Неизвестная команда игнорируется без ответа.
 */
void process_command(const uint8_t *payload, uint16_t len);

/** @brief Таблица команд (константная, размещается в Flash). */
extern const CommandEntry cmd_table[CMD_TABLE_SIZE];

/** @} */
#endif