/*
 * МОДУЛЬ ДИСПЛЕЯ OLED 128x64 v1.7.1
 * Отрисовка интерфейса с графиком влажности и индикаторами
 * ИСПРАВЛЕНО: отображение текста на главном экране
 */

#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include <GyverOLED.h>
#include "config.h"

GyverOLED<SSD1306_128x64, OLED_BUFFER> oled;

#define GRAPH_POINTS 32
#define GRAPH_Y      20
#define GRAPH_H      44

class Display {
private:
  float lastTemp;
  float lastHum;
  uint8_t lastTargetHum;
  bool lastRunning;
  unsigned long lastWorkTime;
  bool lastSensorOK;
  bool lastWaterLow;
  bool lastWindowOpen;
  bool firstDraw;
  
  uint8_t currentBrightness;

  uint8_t humGraph[GRAPH_POINTS];
  uint32_t humState;
  uint8_t gIdx;
  bool gFull;

public:
  Display() : lastTemp(-999), lastHum(-999), lastTargetHum(0),
              lastRunning(false), lastWorkTime(0), lastSensorOK(true),
              lastWaterLow(false), lastWindowOpen(false),
              firstDraw(true), currentBrightness(BRIGHTNESS_FULL),
              gIdx(0), gFull(false), humState(0) {
    memset(humGraph, 0, sizeof(humGraph));
  }

  void begin() {
    oled.init();
    oled.clear();
    
    oled.sendCommand(0xD5);
    oled.sendCommand(0xF0);
    
    setBrightness(BRIGHTNESS_FULL);
  }

  void setBrightness(uint8_t brightness) {
    if (currentBrightness != brightness) {
      currentBrightness = brightness;
      oled.setContrast(brightness);
    }
  }
  
  uint8_t getBrightness() const {
    return currentBrightness;
  }

  void showSplash() {
    oled.clear();
    oled.setScale(2);
    oled.setCursor(0, 2);
    oled.print(F("УВЛАЖНИТЕЛЬ"));
    oled.setScale(1);
    oled.setCursor(30, 6);
    oled.print(F("Версия "));
    oled.print(FIRMWARE_VERSION);
    oled.update();
  }
  
  void showWaterSensorError() {
    oled.clear();
    oled.setScale(1);
    oled.setCursor(10, 2);
    oled.print(F("Датчик воды"));
    oled.setCursor(5, 3);
    oled.print(F("не обнаружен"));
    oled.setCursor(0, 5);
    oled.print(F("Проверьте A0"));
    oled.setCursor(0, 7);
    oled.print(F("Работа без датчика"));
    oled.update();
    delay(3000);
  }

  void addGraphPoint(float humidity, bool running) {
    uint8_t val = (uint8_t)constrain(humidity, 0, 100);
    humGraph[gIdx] = val;
    if (running) humState |=  ((uint32_t)1 << gIdx);
    else         humState &= ~((uint32_t)1 << gIdx);
    gIdx++;
    if (gIdx >= GRAPH_POINTS) {
      gIdx = 0;
      gFull = true;
    }
  }

  void drawGraph() {
    uint8_t cnt = gFull ? GRAPH_POINTS : gIdx;
    
    oled.line(0, GRAPH_Y, 127, GRAPH_Y);
    oled.line(0, 63, 127, 63);
    oled.line(0, GRAPH_Y, 0, 63);
    oled.line(127, GRAPH_Y, 127, 63);
    
    if (cnt == 0) return;

    uint8_t lo = 100, hi = 0;
    for (uint8_t i = 0; i < cnt; i++) {
      uint8_t idx = gFull ? ((gIdx + i) % GRAPH_POINTS) : i;
      uint8_t v = humGraph[idx];
      if (v < lo) lo = v;
      if (v > hi) hi = v;
    }

    if (hi - lo < 5) {
      if (lo >= 3) lo -= 3; else lo = 0;
      if (hi <= 97) hi += 3; else hi = 100;
    }
    if (hi == lo) hi = lo + 1;

    oled.setScale(1);
    oled.setCursor(2, 3);
    oled.print(hi);
    oled.setCursor(2, 7);
    oled.print(lo);

    int16_t prevPx = -1, prevPy = -1;

    for (uint8_t i = 0; i < cnt; i++) {
      uint8_t dataIdx = gFull ? ((gIdx + i) % GRAPH_POINTS) : i;
      uint8_t v = humGraph[dataIdx];

      uint8_t px;
      if (cnt == 1) {
        px = 126;
      } else {
        px = 126 - (uint16_t)(cnt - 1 - i) * 124 / (cnt - 1);
      }

      uint8_t py = 61 - (uint16_t)(v - lo) * (63 - GRAPH_Y - 4) / (hi - lo);
      if (py < GRAPH_Y + 2) py = GRAPH_Y + 2;
      if (py > 61) py = 61;

      if (prevPx >= 0) {
        oled.line(prevPx, prevPy, px, py);
      }
      oled.dot(px, py);

      if (humState & ((uint32_t)1 << dataIdx)) {
        oled.dot(px, 61);
        oled.dot(px, 60);
      }

      prevPx = px;
      prevPy = py;
    }
  }

  // Главный экран с индикаторами
  void drawMainScreen(float temp, float hum, uint8_t targetHum,
                      bool running, unsigned long workTime, bool sensorOK,
                      bool waterLow = false, bool windowOpen = false) {
    bool changed = false;
    if (firstDraw || sensorOK != lastSensorOK) changed = true;
    if (firstDraw || waterLow != lastWaterLow) changed = true;
    if (firstDraw || windowOpen != lastWindowOpen) changed = true;
    if (firstDraw || fabs(temp - lastTemp) >= 0.1) changed = true;
    if (firstDraw || fabs(hum - lastHum) >= 0.1) changed = true;
    if (firstDraw || targetHum != lastTargetHum) changed = true;
    if (firstDraw || running != lastRunning) changed = true;
    if (firstDraw || (workTime / 60) != (lastWorkTime / 60)) changed = true;

    if (!changed) return;

    oled.clear();

    if (!sensorOK) {
      oled.setScale(2);
      oled.setCursor(15, 3);
      oled.print(F("ОШИБКА"));
      oled.setScale(1);
      oled.setCursor(20, 6);
      oled.print(F("Датчик DHT22"));
      oled.update();
      lastSensorOK = sensorOK;
      firstDraw = false;
      return;
    }

    // Верхняя строка: температура и влажность
    oled.setScale(2);
    oled.setCursor(0, 0);
    if (temp >= -40 && temp <= 80) {
      oled.print(temp, 1);
    } else {
      oled.print(F("--"));
    }
    oled.print(F("C"));

    oled.setCursor(9, 0);
    if (hum >= 0 && hum <= 100) {
      oled.print(hum, 1);
    } else {
      oled.print(F("--"));
    }
    oled.print(F("%"));
    
    // Индикаторы статуса (правый верхний угол)
    oled.setScale(1);
    if (waterLow) {
      oled.setCursor(14, 0);
      oled.print(F("W!"));  // Water Low
    }
    if (windowOpen) {
      oled.setCursor(12, 0);
      oled.print(F("O"));   // Open window
    }

    // График
    oled.setScale(1);
    drawGraph();

    oled.update();

    lastTemp = temp;
    lastHum = hum;
    lastTargetHum = targetHum;
    lastRunning = running;
    lastWorkTime = workTime;
    lastSensorOK = sensorOK;
    lastWaterLow = waterLow;
    lastWindowOpen = windowOpen;
    firstDraw = false;
  }

  void drawAboutScreen(unsigned long workTime, uint8_t switchCount,
                       unsigned long totalSwitches) {
    oled.clear();
    oled.setScale(1);
    oled.setCursor(20, 0);
    oled.print(F("О СИСТЕМЕ"));
    oled.line(0, 10, 127, 10);
    oled.setCursor(0, 2);
    oled.print(F("Версия: "));
    oled.print(FIRMWARE_VERSION);
    oled.setCursor(0, 3);
    oled.print(F("Автор: kelll31"));
    oled.setCursor(0, 4);
    oled.print(F("Работа: "));
    if (workTime >= 3600) {
      oled.print(workTime / 3600);
      oled.print(F("ч "));
    }
    oled.print((workTime % 3600) / 60);
    oled.print(F("м"));
    oled.setCursor(0, 5);
    oled.print(F("Перекл:"));
    oled.print(switchCount);
    oled.print(F("/час"));
    oled.setCursor(0, 6);
    oled.print(F("Всего: "));
    oled.print(totalSwitches);
    oled.setCursor(0, 7);
    oled.print(F("ДЛ-выход"));
    oled.update();
  }

  void drawCalibrationScreen(float currentTemp, float currentHum,
                             float tempCal, float humCal, bool editingTemp) {
    oled.clear();
    oled.setScale(1);
    oled.setCursor(15, 0);
    oled.print(F("КАЛИБРОВКА"));
    oled.line(0, 10, 127, 10);
    oled.setCursor(0, 2);
    oled.print(F("Т: "));
    oled.print(currentTemp, 1);
    oled.print(F("C  В: "));
    oled.print(currentHum, 1);
    oled.print(F("%"));
    oled.setCursor(0, 4);
    if (editingTemp) oled.print(F("> "));
    oled.print(F("Корр.Т: "));
    if (tempCal >= 0) oled.print(F("+"));
    oled.print(tempCal, 1);
    oled.print(F("C"));
    oled.setCursor(0, 5);
    if (!editingTemp) oled.print(F("> "));
    oled.print(F("Корр.В: "));
    if (humCal >= 0) oled.print(F("+"));
    oled.print(humCal, 1);
    oled.print(F("%"));
    oled.setCursor(0, 7);
    oled.print(F("ДН-след ДЛ-OK"));
    oled.update();
  }

  // ======================================
  // Методы-обёртки для menu.h
  // ======================================

  void clear() {
    oled.clear();
  }

  void update() {
    oled.update();
  }

  void setCursor(uint8_t x, uint8_t y) {
    oled.setCursor(x, y);
  }

  void print(const char* t) { oled.print(t); }
  void print(const __FlashStringHelper* t) { oled.print(t); }
  void print(int v) { oled.print(v); }
  void print(float v, int d = 1) { oled.print(v, d); }

  void setScale(uint8_t s) {
    oled.setScale(s);
  }

  void drawLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
    oled.line(x0, y0, x1, y1);
  }

  void drawRect(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, bool fill = false) {
    if (fill) {
      oled.rect(x0, y0, x1, y1, OLED_FILL);
    } else {
      oled.line(x0, y0, x1, y0);
      oled.line(x0, y1, x1, y1);
      oled.line(x0, y0, x0, y1);
      oled.line(x1, y0, x1, y1);
    }
  }

  void invertRect(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
    oled.rect(x0, y0, x1, y1, OLED_FILL);
  }
};

#endif // DISPLAY_H
