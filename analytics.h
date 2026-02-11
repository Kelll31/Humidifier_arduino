/*
 * МОДУЛЬ АНАЛИТИКИ
 * Расширенная статистика, детектор окна, датчик воды, адаптивное обучение
 */

#ifndef ANALYTICS_H
#define ANALYTICS_H

#include <Arduino.h>
#include <EEPROM.h>
#include "config.h"

// Структура статистики за час (оптимизировано - 4 байта)
struct HourlyStats {
  uint8_t avgTemp;      // Средняя температура (0-100, смещение +50)
  uint8_t avgHum;       // Средняя влажность (0-100)
  uint8_t runTime;      // Время работы в минутах (0-60)
  uint8_t switches;     // Количество переключений
};

class Analytics {
private:
  // Статистика (хранится в RAM для текущего часа)
  uint8_t currentHour;
  uint16_t tempSum;
  uint16_t humSum;
  uint8_t sampleCount;
  uint8_t hourRunTime;
  uint8_t hourSwitches;
  
  // Детектор окна
  float baselineTemp;
  uint8_t tempDropCount;
  bool windowOpen;
  unsigned long lastWindowCheck;
  
  // Датчик воды
  bool waterLow;
  unsigned long lastWaterCheck;
  
  // Адаптивное обучение (минимальное использование RAM)
  uint8_t learnedMinHum;
  uint8_t learnedMaxHum;
  bool learningEnabled;

public:
  Analytics() : currentHour(255), tempSum(0), humSum(0), sampleCount(0),
                hourRunTime(0), hourSwitches(0), baselineTemp(20.0),
                tempDropCount(0), windowOpen(false), lastWindowCheck(0),
                waterLow(false), lastWaterCheck(0), learnedMinHum(0),
                learnedMaxHum(0), learningEnabled(LEARNING_ENABLED) {}

  // Инициализация
  void begin() {
    pinMode(WATER_LEVEL_PIN, INPUT);
    loadLearningData();
  }

  // ========================================
  // ДАТЧИК ВОДЫ
  // ========================================
  
  bool checkWaterLevel() {
    // Проверка каждые 5 секунд
    if (millis() - lastWaterCheck < 5000) {
      return !waterLow;
    }
    
    lastWaterCheck = millis();
    
    // Резистивный датчик: чем больше воды - тем меньше сопротивление
    int waterLevel = analogRead(WATER_LEVEL_PIN);
    waterLow = (waterLevel < WATER_THRESHOLD);
    
    return !waterLow;
  }
  
  bool isWaterLow() const {
    return waterLow;
  }

  // ========================================
  // ДЕТЕКТОР ОТКРЫТОГО ОКНА
  // ========================================
  
  void updateWindowDetector(float currentTemp) {
    unsigned long now = millis();
    
    if (now - lastWindowCheck < WINDOW_CHECK_INTERVAL) {
      return;
    }
    
    lastWindowCheck = now;
    
    // Проверка резкого падения температуры
    if (baselineTemp - currentTemp >= WINDOW_TEMP_DROP) {
      tempDropCount++;
      if (tempDropCount >= WINDOW_TEMP_SAMPLES) {
        windowOpen = true;
      }
    } else {
      // Сброс счетчика если температура восстановилась
      if (currentTemp >= baselineTemp - 0.5) {
        tempDropCount = 0;
        windowOpen = false;
        baselineTemp = currentTemp; // Обновляем базовую линию
      }
    }
  }
  
  bool isWindowOpen() const {
    return windowOpen;
  }

  // ========================================
  // РАСШИРЕННАЯ СТАТИСТИКА
  // ========================================
  
  void addSample(float temp, float hum, bool running) {
    uint8_t hour = (millis() / 3600000UL) % 24; // Текущий час
    
    // Новый час - сохраняем предыдущий
    if (hour != currentHour && sampleCount > 0) {
      saveHourlyStats();
      currentHour = hour;
      tempSum = 0;
      humSum = 0;
      sampleCount = 0;
      hourRunTime = 0;
      hourSwitches = 0;
    }
    
    currentHour = hour;
    
    // Накапливаем данные
    tempSum += (uint8_t)constrain(temp + 50, 0, 100); // Смещение для отрицательных температур
    humSum += (uint8_t)constrain(hum, 0, 100);
    sampleCount++;
    
    if (running) {
      hourRunTime++; // Каждый сэмпл = 2 сек
    }
  }
  
  void incrementSwitches() {
    hourSwitches++;
  }
  
  void saveHourlyStats() {
    if (sampleCount == 0) return;
    
    HourlyStats stats;
    stats.avgTemp = tempSum / sampleCount;
    stats.avgHum = humSum / sampleCount;
    stats.runTime = min(hourRunTime / 30, 60); // Перевод из сэмплов в минуты
    stats.switches = min(hourSwitches, 255);
    
    // Сохраняем в EEPROM (кольцевой буфер)
    int addr = EEPROM_STATS_ADDR + (currentHour * sizeof(HourlyStats));
    EEPROM.put(addr, stats);
  }
  
  HourlyStats getHourStats(uint8_t hoursAgo) {
    uint8_t hour = ((millis() / 3600000UL) - hoursAgo) % 24;
    int addr = EEPROM_STATS_ADDR + (hour * sizeof(HourlyStats));
    HourlyStats stats;
    EEPROM.get(addr, stats);
    return stats;
  }

  // ========================================
  // АДАПТИВНОЕ ОБУЧЕНИЕ
  // ========================================
  
  void updateLearning() {
    if (!learningEnabled) return;
    
    // Анализ последних 24 часов
    uint16_t totalHum = 0;
    uint8_t validSamples = 0;
    
    for (uint8_t i = 0; i < min(24, STATS_HISTORY_SIZE); i++) {
      HourlyStats stats = getHourStats(i);
      if (stats.avgHum > 0 && stats.avgHum <= 100) {
        totalHum += stats.avgHum;
        validSamples++;
      }
    }
    
    if (validSamples >= LEARNING_MIN_DATA) {
      uint8_t avgHum = totalHum / validSamples;
      
      // Плавная корректировка целевых значений
      learnedMinHum = constrain(avgHum - 10, 30, 70);
      learnedMaxHum = constrain(avgHum + 10, 40, 80);
      
      saveLearningData();
    }
  }
  
  void saveLearningData() {
    EEPROM.update(EEPROM_LEARNING_ADDR, learnedMinHum);
    EEPROM.update(EEPROM_LEARNING_ADDR + 1, learnedMaxHum);
  }
  
  void loadLearningData() {
    learnedMinHum = EEPROM.read(EEPROM_LEARNING_ADDR);
    learnedMaxHum = EEPROM.read(EEPROM_LEARNING_ADDR + 1);
    
    // Проверка корректности
    if (learnedMinHum < 20 || learnedMinHum > 80 ||
        learnedMaxHum < 30 || learnedMaxHum > 90 ||
        learnedMaxHum <= learnedMinHum) {
      learnedMinHum = 0;
      learnedMaxHum = 0;
    }
  }
  
  uint8_t getLearnedMin() const {
    return learnedMinHum;
  }
  
  uint8_t getLearnedMax() const {
    return learnedMaxHum;
  }
  
  bool hasLearnedData() const {
    return learnedMinHum > 0 && learnedMaxHum > learnedMinHum;
  }
  
  void enableLearning(bool enable) {
    learningEnabled = enable;
  }
  
  bool isLearningEnabled() const {
    return learningEnabled;
  }
};

#endif // ANALYTICS_H
