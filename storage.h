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
  unsigned long totalSwitches; // Общее количество переключений
  
  // Защита от износа EEPROM
  bool needsSave;
  unsigned long lastSaveTime;

public:
  Storage() : minHumidity(DEFAULT_MIN_HUMIDITY),
              maxHumidity(DEFAULT_MAX_HUMIDITY),
              hysteresis(DEFAULT_HYSTERESIS),
              tempCalibration(TEMP_CALIBRATION),
              humCalibration(HUM_CALIBRATION),
              workTime(0),
              totalSwitches(0),
              needsSave(false),
              lastSaveTime(0) {}

  // Инициализация и загрузка настроек
  void begin() {
    // Проверка магического числа
    uint8_t magic = EEPROM.read(EEPROM_MAGIC_ADDR);

    if (magic != EEPROM_MAGIC_VALUE) {
      // Первый запуск - инициализация EEPROM
      setDefaults();
      saveDirect();
    } else {
      // Загрузка настроек
      load();
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
    
    // Загрузка общего количества переключений
    EEPROM.get(EEPROM_TOTAL_SWITCHES_ADDR, totalSwitches);

    // Валидация значений
    validateSettings();
  }

  // Валидация загруженных настроек
  void validateSettings() {
    // Проверка влажности
    if (minHumidity < 20 || minHumidity > 80) minHumidity = DEFAULT_MIN_HUMIDITY;
    if (maxHumidity < 30 || maxHumidity > 90) maxHumidity = DEFAULT_MAX_HUMIDITY;
    if (hysteresis < 1 || hysteresis > 20) hysteresis = DEFAULT_HYSTERESIS;
    if (minHumidity >= maxHumidity) {
      minHumidity = DEFAULT_MIN_HUMIDITY;
      maxHumidity = DEFAULT_MAX_HUMIDITY;
    }

    // Проверка калибровки (на NaN и диапазон)
    if (isnan(tempCalibration) || tempCalibration < -10.0 || tempCalibration > 10.0) {
      tempCalibration = TEMP_CALIBRATION;
    }
    if (isnan(humCalibration) || humCalibration < -20.0 || humCalibration > 20.0) {
      humCalibration = HUM_CALIBRATION;
    }

    // Проверка времени работы (не должно быть слишком большим)
    if (workTime > 0xFFFFFFF) workTime = 0; // Сброс при невалидных значениях
    if (totalSwitches > 0xFFFFFFF) totalSwitches = 0;
  }

  // Сохранение настроек в EEPROM (с задержкой)
  void save() {
    needsSave = true;
  }

  // Непосредственное сохранение (без задержки)
  void saveDirect() {
    EEPROM.write(EEPROM_MAGIC_ADDR, EEPROM_MAGIC_VALUE);
    EEPROM.write(EEPROM_MIN_HUM_ADDR, minHumidity);
    EEPROM.write(EEPROM_MAX_HUM_ADDR, maxHumidity);
    EEPROM.write(EEPROM_HYSTERESIS_ADDR, hysteresis);

    EEPROM.put(EEPROM_TEMP_CAL_ADDR, tempCalibration);
    EEPROM.put(EEPROM_HUM_CAL_ADDR, humCalibration);
    EEPROM.put(EEPROM_WORK_TIME_ADDR, workTime);
    EEPROM.put(EEPROM_TOTAL_SWITCHES_ADDR, totalSwitches);

    needsSave = false;
    lastSaveTime = millis();
  }

  // Проверка и сохранение (вызывать в loop)
  void tick() {
    // Сохраняем не чаще раза в минуту для защиты EEPROM
    if (needsSave && (millis() - lastSaveTime >= 60000)) {
      saveDirect();
    }
  }

  // Установка значений по умолчанию
  void setDefaults() {
    minHumidity = DEFAULT_MIN_HUMIDITY;
    maxHumidity = DEFAULT_MAX_HUMIDITY;
    hysteresis = DEFAULT_HYSTERESIS;
    tempCalibration = TEMP_CALIBRATION;
    humCalibration = HUM_CALIBRATION;
    workTime = 0;
    totalSwitches = 0;
  }

  // Сброс всех настроек
  void reset() {
    setDefaults();
    saveDirect();
  }

  // Геттеры
  uint8_t getMinHumidity() const { return minHumidity; }
  uint8_t getMaxHumidity() const { return maxHumidity; }
  uint8_t getHysteresis() const { return hysteresis; }
  float getTempCalibration() const { return tempCalibration; }
  float getHumCalibration() const { return humCalibration; }
  unsigned long getWorkTime() const { return workTime; }
  unsigned long getTotalSwitches() const { return totalSwitches; }

  // Сеттеры
  void setMinHumidity(uint8_t value) {
    uint8_t newValue = constrain(value, 20, 80);
    if (newValue != minHumidity) {
      minHumidity = newValue;
      save();
    }
  }

  void setMaxHumidity(uint8_t value) {
    uint8_t newValue = constrain(value, 30, 90);
    if (newValue != maxHumidity) {
      maxHumidity = newValue;
      save();
    }
  }

  void setHysteresis(uint8_t value) {
    uint8_t newValue = constrain(value, 1, 20);
    if (newValue != hysteresis) {
      hysteresis = newValue;
      save();
    }
  }

  void setTempCalibration(float value) {
    float newValue = constrain(value, -10.0, 10.0);
    if (abs(newValue - tempCalibration) > 0.01) {
      tempCalibration = newValue;
      save();
    }
  }

  void setHumCalibration(float value) {
    float newValue = constrain(value, -20.0, 20.0);
    if (abs(newValue - humCalibration) > 0.01) {
      humCalibration = newValue;
      save();
    }
  }

  // Увеличение времени работы
  void incrementWorkTime(unsigned long seconds) {
    // Защита от переполнения
    if (workTime < 0xFFFFFFFF - seconds) {
      workTime += seconds;
    }
  }

  // Увеличение счетчика переключений
  void incrementSwitchCount() {
    if (totalSwitches < 0xFFFFFFFF) {
      totalSwitches++;
    }
  }

  // Сброс времени работы
  void resetWorkTime() {
    workTime = 0;
    save();
  }

  // Сброс счетчика переключений
  void resetSwitchCount() {
    totalSwitches = 0;
    save();
  }

  // Форматирование времени работы
  void formatWorkTime(char* buffer, size_t bufferSize) {
    unsigned long hours = workTime / 3600;
    unsigned long minutes = (workTime % 3600) / 60;

    if (hours > 0) {
      snprintf(buffer, bufferSize, "%luч %luм", hours, minutes);
    } else {
      snprintf(buffer, bufferSize, "%luм", minutes);
    }
  }
};

#endif // STORAGE_H
