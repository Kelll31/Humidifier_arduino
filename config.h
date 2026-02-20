/*
 * КОНФИГУРАЦИЯ СИСТЕМЫ v2.2
 * Оптимизировано для Arduino Nano
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <avr/wdt.h>

// ============================================================================
// ПИНЫ ПОДКЛЮЧЕНИЯ
// ============================================================================

#define DHT_PIN           6
#define DHT_TYPE          DHT22

#define OLED_ADDRESS      0x3C

#define ENCODER_CLK       2
#define ENCODER_DT        3
#define ENCODER_SW        4

#define HUMIDIFIER_PIN    7

// ============================================================================
// ДАТЧИК УРОВНЯ ВОДЫ
// ============================================================================

#define WATER_SENSOR_ENABLED      true
#define WATER_LEVEL_PIN           A0

#define WATER_LEVEL_EMPTY         100
#define WATER_LEVEL_LOW           250
#define WATER_LEVEL_MEDIUM        400
#define WATER_LEVEL_HIGH          550
#define WATER_LEVEL_FULL          700

#define WATER_THRESHOLD           WATER_LEVEL_LOW

#define WATER_SENSOR_MIN         30
#define WATER_SENSOR_MAX         900

// ============================================================================
// НАСТРОЙКИ ПО УМОЛЧАНИЮ
// ============================================================================

#define DEFAULT_MIN_HUMIDITY    40
#define DEFAULT_MAX_HUMIDITY    60
#define DEFAULT_HYSTERESIS      5

#define UPDATE_INTERVAL         2000
#define AUTOSAVE_INTERVAL       300000
#define MIN_RUN_TIME            30000
#define MIN_PAUSE_TIME          60000

#define MAX_SWITCHES_PER_HOUR   10

#define TEMP_CALIBRATION        0.0
#define HUM_CALIBRATION         0.0

// ============================================================================
// ДЕТЕКТОР ОТКРЫТОГО ОКНА
// ============================================================================

#define WINDOW_DETECTOR_ENABLED true
#define WINDOW_TEMP_DROP        2.0
#define WINDOW_CHECK_INTERVAL   30000
#define WINDOW_TEMP_SAMPLES     3

// ============================================================================
// СТАТИСТИКА И ОБУЧЕНИЕ
// ============================================================================

#define STATS_ENABLED           true
#define STATS_HISTORY_SIZE      24

#define LEARNING_ENABLED        true
#define LEARNING_MIN_DATA       24

// ============================================================================
// НАСТРОЙКИ ДИСПЛЕЯ
// ============================================================================

#define SCREEN_WIDTH            128
#define SCREEN_HEIGHT           64
#define SCREEN_TIMEOUT          30000

#define BRIGHTNESS_FULL         255
#define BRIGHTNESS_DIM1         191
#define BRIGHTNESS_DIM2         51
#define DIM_TIMEOUT_1           10000
#define DIM_TIMEOUT_2           180000

// ============================================================================
// НАСТРОЙКИ МЕНЮ
// ============================================================================

#define MENU_ITEMS              8
#define LONG_PRESS_TIME         2000
#define ENCODER_FAST_THRESHOLD  50

// ============================================================================
// АДРЕСА EEPROM
// ============================================================================

#define EEPROM_MAGIC_ADDR          0
#define EEPROM_MAGIC_VALUE         0xAE
#define EEPROM_MIN_HUM_ADDR        1
#define EEPROM_MAX_HUM_ADDR        2
#define EEPROM_HYSTERESIS_ADDR     3
#define EEPROM_TEMP_CAL_ADDR       4
#define EEPROM_HUM_CAL_ADDR        8
#define EEPROM_WORK_TIME_ADDR      12
#define EEPROM_TOTAL_SWITCHES_ADDR 16
#define EEPROM_WATER_THRESHOLD_ADDR 20
#define EEPROM_LEARNING_ADDR       22
#define EEPROM_STATS_ADDR          200

// ============================================================================
// СИСТЕМНЫЕ КОНСТАНТЫ
// ============================================================================

#define FIRMWARE_VERSION        "2.2"

#endif // CONFIG_H
