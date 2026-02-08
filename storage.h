/*
 * МОДУЛЬ ХРАНИЛИЩА НАСТРОЕК
 * Работа с EEPROM для сохранения параметров
 */

#ifndef STORAGE_H
#define STORAGE_H

#include <Arduino.h>
#include <EEPROM.h>
#include "config.h"

class Storage {
private:
  uint8_t minHumidity;
  uint8_t maxHumidity;
  uint8_t hysteresis;
  float tempCalibration;
  float humCalibration;
  unsigned long workTime; // Время работы в секундах

public:
  Storage() : minHumidity(DEFAULT_MIN_HUMIDITY),
              maxHumidity(DEFAULT_MAX_HUMIDITY),
              hysteresis(DEFAULT_HYSTERESIS),
              tempCalibration(TEMP_CALIBRATION),
              humCalibration(HUM_CALIBRATION),
              workTime(0) {}

  // Инициализация и загрузка настроек
  void begin() {
    // Проверка магического числа
    uint8_t magic = EEPROM.read(EEPROM_MAGIC_ADDR);

    if (magic != EEPROM_MAGIC_VALUE) {
      // Первый запуск - инициализация EEPROM
      DEBUG_PRINTLN(F("[Storage] Первый запуск. Инициализация..."));
      setDefaults();
      save();
    } else {
      // Загрузка настроек
      load();
      DEBUG_PRINTLN(F("[Storage] Настройки загружены из EEPROM"));
    }
  }

  // Загрузка настроек из EEPROM
  void load() {
    minHumidity = EEPROM.read(EEPROM_MIN_HUM_ADDR);
    maxHumidity = EEPROM.read(EEPROM_MAX_HUM_ADDR);
    hysteresis = EEPROM.read(EEPROM_HYSTERESIS_ADDR);

    // Загрузка калибровки (float = 4 байта)
    EEPROM.get(EEPROM_TEMP_CAL_ADDR, tempCalibration);
    EEPROM.get(EEPROM_HUM_CAL_ADDR, humCalibration);

    // Загрузка времени работы
    EEPROM.get(EEPROM_WORK_TIME_ADDR, workTime);

    // Валидация значений
    if (minHumidity < 20 || minHumidity > 80) minHumidity = DEFAULT_MIN_HUMIDITY;
    if (maxHumidity < 30 || maxHumidity > 90) maxHumidity = DEFAULT_MAX_HUMIDITY;
    if (hysteresis < 1 || hysteresis > 20) hysteresis = DEFAULT_HYSTERESIS;
    if (minHumidity >= maxHumidity) {
      minHumidity = DEFAULT_MIN_HUMIDITY;
      maxHumidity = DEFAULT_MAX_HUMIDITY;
    }
  }

  // Сохранение настроек в EEPROM
  void save() {
    EEPROM.write(EEPROM_MAGIC_ADDR, EEPROM_MAGIC_VALUE);
    EEPROM.write(EEPROM_MIN_HUM_ADDR, minHumidity);
    EEPROM.write(EEPROM_MAX_HUM_ADDR, maxHumidity);
    EEPROM.write(EEPROM_HYSTERESIS_ADDR, hysteresis);

    EEPROM.put(EEPROM_TEMP_CAL_ADDR, tempCalibration);
    EEPROM.put(EEPROM_HUM_CAL_ADDR, humCalibration);
    EEPROM.put(EEPROM_WORK_TIME_ADDR, workTime);

    DEBUG_PRINTLN(F("[Storage] Настройки сохранены в EEPROM"));
  }

  // Установка значений по умолчанию
  void setDefaults() {
    minHumidity = DEFAULT_MIN_HUMIDITY;
    maxHumidity = DEFAULT_MAX_HUMIDITY;
    hysteresis = DEFAULT_HYSTERESIS;
    tempCalibration = TEMP_CALIBRATION;
    humCalibration = HUM_CALIBRATION;
    workTime = 0;

    DEBUG_PRINTLN(F("[Storage] Установлены значения по умолчанию"));
  }

  // Сброс всех настроек
  void reset() {
    setDefaults();
    save();
    DEBUG_PRINTLN(F("[Storage] Все настройки сброшены"));
  }

  // Геттеры
  uint8_t getMinHumidity() const { return minHumidity; }
  uint8_t getMaxHumidity() const { return maxHumidity; }
  uint8_t getHysteresis() const { return hysteresis; }
  float getTempCalibration() const { return tempCalibration; }
  float getHumCalibration() const { return humCalibration; }
  unsigned long getWorkTime() const { return workTime; }

  // Сеттеры
  void setMinHumidity(uint8_t value) {
    minHumidity = constrain(value, 20, 80);
    DEBUG_PRINT(F("[Storage] Мин. влажность: "));
    DEBUG_PRINTLN(minHumidity);
  }

  void setMaxHumidity(uint8_t value) {
    maxHumidity = constrain(value, 30, 90);
    DEBUG_PRINT(F("[Storage] Макс. влажность: "));
    DEBUG_PRINTLN(maxHumidity);
  }

  void setHysteresis(uint8_t value) {
    hysteresis = constrain(value, 1, 20);
    DEBUG_PRINT(F("[Storage] Гистерезис: "));
    DEBUG_PRINTLN(hysteresis);
  }

  void setTempCalibration(float value) {
    tempCalibration = constrain(value, -10.0, 10.0);
    DEBUG_PRINT(F("[Storage] Калибровка температуры: "));
    DEBUG_PRINTLN(tempCalibration);
  }

  void setHumCalibration(float value) {
    humCalibration = constrain(value, -20.0, 20.0);
    DEBUG_PRINT(F("[Storage] Калибровка влажности: "));
    DEBUG_PRINTLN(humCalibration);
  }

  // Увеличение времени работы
  void incrementWorkTime(unsigned long seconds) {
    workTime += seconds;
  }

  // Сброс времени работы
  void resetWorkTime() {
    workTime = 0;
    DEBUG_PRINTLN(F("[Storage] Время работы сброшено"));
  }

  // Форматирование времени работы
  void formatWorkTime(char* buffer, size_t bufferSize) {
    unsigned long hours = workTime / 3600;
    unsigned long minutes = (workTime % 3600) / 60;

    if (hours > 0) {
      snprintf(buffer, bufferSize, "%luч %luмин", hours, minutes);
    } else {
      snprintf(buffer, bufferSize, "%luмин", minutes);
    }
  }
};

#endif // STORAGE_H
