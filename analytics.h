/*
 * МОДУЛЬ АНАЛИТИКИ v1.8
 * Датчик воды, детектора окна, статистика
 */

#ifndef ANALYTICS_H
#define ANALYTICS_H

#include <Arduino.h>
#include <EEPROM.h>
#include "config.h"

class Analytics {
private:
  uint8_t currentHour;
  uint16_t tempSum;
  uint16_t humSum;
  uint8_t sampleCount;
  uint8_t hourRunTime;
  
  float baselineTemp;
  uint8_t tempDropCount;
  bool windowOpen;
  unsigned long lastWindowCheck;
  
  bool waterLow;
  bool waterSensorPresent;
  unsigned long lastWaterCheck;
  uint8_t waterStableCount;
  int lastWaterValue;
  uint16_t waterThreshold;

public:
  Analytics() : currentHour(255), tempSum(0), humSum(0), sampleCount(0),
                hourRunTime(0), baselineTemp(20.0), tempDropCount(0),
                windowOpen(false), lastWindowCheck(0), waterLow(false),
                waterSensorPresent(false), lastWaterCheck(0), waterStableCount(0),
                lastWaterValue(0), waterThreshold(WATER_THRESHOLD) {}

  void begin() {
    pinMode(WATER_LEVEL_PIN, INPUT);
    
    uint8_t saved = EEPROM.read(EEPROM_WATER_THRESHOLD_ADDR);
    if (saved >= 30 && saved <= 900) waterThreshold = saved;
    
    uint32_t sum = 0;
    for (uint8_t i = 0; i < 4; i++) {
      sum += analogRead(WATER_LEVEL_PIN);
      delay(5);
    }
    int avg = sum / 4;
    
    if (avg >= WATER_SENSOR_MIN && avg <= WATER_SENSOR_MAX) {
      waterSensorPresent = true;
      waterLow = (avg < waterThreshold);
    } else {
      waterSensorPresent = false;
      waterLow = false;
    }
    lastWaterValue = avg;
  }

  void saveWaterThreshold(uint16_t t) {
    waterThreshold = t;
    EEPROM.update(EEPROM_WATER_THRESHOLD_ADDR, (uint8_t)t);
  }
  uint16_t getWaterThreshold() const { return waterThreshold; }

  int readWaterSensor() {
    uint32_t sum = 0;
    for (uint8_t i = 0; i < 4; i++) {
      sum += analogRead(WATER_LEVEL_PIN);
      delayMicroseconds(100);
    }
    return sum / 4;
  }
  
  uint8_t getWaterPercent() const {
    if (!waterSensorPresent) return 255;
    int v = lastWaterValue;
    if (v <= WATER_LEVEL_EMPTY) return 0;
    if (v >= WATER_LEVEL_FULL) return 100;
    if (v < WATER_LEVEL_LOW) return map(v, WATER_LEVEL_EMPTY, WATER_LEVEL_LOW, 0, 25);
    if (v < WATER_LEVEL_MEDIUM) return map(v, WATER_LEVEL_LOW, WATER_LEVEL_MEDIUM, 25, 50);
    if (v < WATER_LEVEL_HIGH) return map(v, WATER_LEVEL_MEDIUM, WATER_LEVEL_HIGH, 50, 75);
    return map(v, WATER_LEVEL_HIGH, WATER_LEVEL_FULL, 75, 100);
  }
  
  int getWaterRawValue() const { return lastWaterValue; }
  
  bool checkWaterLevel() {
    if (!waterSensorPresent) return true;
    if (millis() - lastWaterCheck < 1000) return !waterLow;
    
    lastWaterCheck = millis();
    int level = readWaterSensor();
    lastWaterValue = level;
    
    bool nowLow = (level < waterThreshold);
    if (nowLow != waterLow) {
      waterStableCount++;
      if (waterStableCount >= 3) { waterLow = nowLow; waterStableCount = 0; }
    } else {
      waterStableCount = 0;
    }
    return !waterLow;
  }
  
  bool isWaterLow() const { return waterLow && waterSensorPresent; }
  bool isWaterSensorPresent() const { return waterSensorPresent; }

  void updateWindowDetector(float temp) {
    unsigned long now = millis();
    if (now - lastWindowCheck < WINDOW_CHECK_INTERVAL) return;
    lastWindowCheck = now;
    
    if (baselineTemp - temp >= WINDOW_TEMP_DROP) {
      tempDropCount++;
      if (tempDropCount >= WINDOW_TEMP_SAMPLES) windowOpen = true;
    } else {
      if (temp >= baselineTemp - 0.5) { tempDropCount = 0; windowOpen = false; baselineTemp = temp; }
    }
  }
  
  bool isWindowOpen() const { return windowOpen; }

  void addSample(float temp, float hum, bool running) {
    uint8_t hour = (millis() / 3600000UL) % 24;
    if (hour != currentHour && sampleCount > 0) {
      saveHourlyStats();
      currentHour = hour; tempSum = 0; humSum = 0; sampleCount = 0; hourRunTime = 0;
    }
    currentHour = hour;
    tempSum += (uint8_t)constrain(temp + 50, 0, 100);
    humSum += (uint8_t)constrain(hum, 0, 100);
    sampleCount++;
    if (running) hourRunTime++;
  }
  
  void saveHourlyStats() {
    if (sampleCount == 0) return;
    struct Stats { uint8_t t, h, r, s; };
    Stats s;
    s.t = tempSum / sampleCount;
    s.h = humSum / sampleCount;
    s.r = min(hourRunTime / 30, 60);
    s.s = 0;
    int addr = EEPROM_STATS_ADDR + (currentHour * 4);
    EEPROM.put(addr, s);
  }
};

#endif // ANALYTICS_H
