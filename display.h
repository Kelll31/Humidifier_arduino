/*
 * МОДУЛЬ ДИСПЛЕЯ OLED 128x64
 * Отрисовка интерфейса с графиком влажности
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
  bool firstDraw;
  
  uint8_t currentBrightness;

  uint8_t humGraph[GRAPH_POINTS];
  uint32_t humState;
  uint8_t gIdx;
  bool gFull;

public:
  Display() : lastTemp(-999), lastHum(-999), lastTargetHum(0),
              lastRunning(false), lastWorkTime(0), lastSensorOK(true),
              firstDraw(true), currentBrightness(BRIGHTNESS_FULL),
              gIdx(0), gFull(false), humState(0) {
    memset(humGraph, 0, sizeof(humGraph));
  }

  void begin() {
    oled.init();
    oled.clear();
    
    // Настройка частоты осциллятора дисплея для уменьшения гудения
    // Команда 0xD5 устанавливает частоту обновления дисплея
    // Формат: 0xD5, затем [7:4]=частота осциллятора, [3:0]=делитель
    // 0xF0 = максимальная частота осциллятора (Fosc), делитель=0 (DCLK=Fosc)
    // Это повышает частоту обновления и устраняет гудение
    oled.sendCommand(0xD5);  // Set Display Clock Divide Ratio/Oscillator Frequency
    oled.sendCommand(0xF0);  // Максимальная частота: Fosc на максимум, делитель = 1
    
    setBrightness(BRIGHTNESS_FULL);
  }

  // Установка яркости дисплея
  void setBrightness(uint8_t brightness) {
    if (currentBrightness != brightness) {
      currentBrightness = brightness;
      oled.setContrast(brightness);
    }
  }
  
  // Получение текущей яркости
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
    
    // Рамка графика из 4 линий (всегда рисуем)
    oled.line(0, GRAPH_Y, 127, GRAPH_Y);              // верх
    oled.line(0, 63, 127, 63);                        // низ
    oled.line(0, GRAPH_Y, 0, 63);                     // лево
    oled.line(127, GRAPH_Y, 127, 63);                 // право
    
    if (cnt == 0) return;

    // Находим min/max
    uint8_t lo = 100, hi = 0;
    for (uint8_t i = 0; i < cnt; i++) {
      uint8_t idx = gFull ? ((gIdx + i) % GRAPH_POINTS) : i;
      uint8_t v = humGraph[idx];
      if (v < lo) lo = v;
      if (v > hi) hi = v;
    }

    // Диапазон минимум 5%
    if (hi - lo < 5) {
      if (lo >= 3) lo -= 3; else lo = 0;
      if (hi <= 97) hi += 3; else hi = 100;
    }
    if (hi == lo) hi = lo + 1;

    // Мин/макс метки
    oled.setScale(1);
    oled.setCursor(2, 3);    // строка 3 = y24 (под температурой)
    oled.print(hi);
    oled.setCursor(2, 7);    // строка 7 = y56
    oled.print(lo);

    // Рисуем линию графика (новые данные справа)
    int16_t prevPx = -1, prevPy = -1;

    for (uint8_t i = 0; i < cnt; i++) {
      uint8_t dataIdx = gFull ? ((gIdx + i) % GRAPH_POINTS) : i;
      uint8_t v = humGraph[dataIdx];

      // X: прижать к правому краю
      uint8_t px;
      if (cnt == 1) {
        px = 126;
      } else {
        px = 126 - (uint16_t)(cnt - 1 - i) * 124 / (cnt - 1);
      }

      // Y: мапим значение в пиксели (GRAPH_Y+2 .. 61)
      uint8_t py = 61 - (uint16_t)(v - lo) * (63 - GRAPH_Y - 4) / (hi - lo);
      if (py < GRAPH_Y + 2) py = GRAPH_Y + 2;
      if (py > 61) py = 61;

      if (prevPx >= 0) {
        oled.line(prevPx, prevPy, px, py);
      }
      oled.dot(px, py);

      // Отметки включения увлажнителя
      if (humState & ((uint32_t)1 << dataIdx)) {
        oled.dot(px, 61);
        oled.dot(px, 60);
      }

      prevPx = px;
      prevPy = py;
    }
  }

  // Главный экран
  void drawMainScreen(float temp, float hum, uint8_t targetHum,
                      bool running, unsigned long workTime, bool sensorOK) {
    bool changed = false;
    if (firstDraw || sensorOK != lastSensorOK) changed = true;
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
    oled.setCursorXY(0, 0);
    if (temp >= -40 && temp <= 80) {
      oled.print(temp, 1);
    } else {
      oled.print(F("--"));
    }
    oled.print(F("C"));

    oled.setCursorXY(70, 0);
    if (hum >= 0 && hum <= 100) {
      oled.print(hum, 1);
    } else {
      oled.print(F("--"));
    }
    oled.print(F("%"));

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
    firstDraw = false;
  }

  // Экран "О системе"
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

  // Экран калибровки
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
    // НЕ вызываем oled.update() здесь!
    // Меню сначала рисует, потом вызывает update()
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
      oled.line(x0, y0, x1, y0);  // верх
      oled.line(x0, y1, x1, y1);  // низ
      oled.line(x0, y0, x0, y1);  // лево
      oled.line(x1, y0, x1, y1);  // право
    }
  }

  void invertRect(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
    oled.rect(x0, y0, x1, y1, OLED_FILL);
  }
};

#endif // DISPLAY_H
