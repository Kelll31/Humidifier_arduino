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
#define GRAPH_Y      16
#define GRAPH_H      48

class Display {
private:
  float lastTemp;
  float lastHum;
  uint8_t lastTargetHum;
  bool lastRunning;
  unsigned long lastWorkTime;
  bool lastSensorOK;
  bool firstDraw;

  uint8_t humGraph[GRAPH_POINTS];
  uint32_t humState;
  uint8_t gIdx;
  bool gFull;

public:
  Display() : lastTemp(-999), lastHum(-999), lastTargetHum(0),
              lastRunning(false), lastWorkTime(0), lastSensorOK(true),
              firstDraw(true), gIdx(0), gFull(false), humState(0) {
    memset(humGraph, 0, sizeof(humGraph));
  }

  void begin() {
    oled.init();
    oled.clear();
    oled.setContrast(255);
  }

  void showSplash() {
    oled.clear();
    oled.setScale(2);
    oled.setCursor(10, 2);
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

    // Координаты области графика
    const uint8_t gx = 0;
    const uint8_t gy = GRAPH_Y;
    const uint8_t gw = 128;
    const uint8_t gh = GRAPH_H;

    // Рамка из линий (не заливкой!)
    oled.line(gx, gy, gx + gw - 1, gy);                // верх
    oled.line(gx, gy + gh - 1, gx + gw - 1, gy + gh - 1); // низ
    oled.line(gx, gy, gx, gy + gh - 1);                // лево
    oled.line(gx + gw - 1, gy, gx + gw - 1, gy + gh - 1); // право

    // Мин/макс метки
    oled.setScale(1);
    oled.setCursor(2, (gy + 2) / 8);
    oled.print(hi);
    oled.setCursor(2, (gy + gh - 10) / 8);
    oled.print(lo);

    // Рисуем линию графика
    // Новые данные справа, старые слева
    // График прижат к правому краю
    int16_t prevPx = -1, prevPy = -1;

    for (uint8_t i = 0; i < cnt; i++) {
      // i=0 старейшая, i=cnt-1 новейшая
      uint8_t dataIdx = gFull ? ((gIdx + i) % GRAPH_POINTS) : i;
      uint8_t v = humGraph[dataIdx];

      // X: прижать к правому краю
      // самая новая точка (i=cnt-1) -> x = gx+gw-2
      // самая старая (i=0) -> x = gx+gw-2 - (cnt-1)*step
      uint8_t px;
      if (cnt == 1) {
        px = gx + gw - 2;
      } else {
        px = gx + gw - 2 - (uint16_t)(cnt - 1 - i) * (gw - 4) / (cnt - 1);
      }

      // Y: мапим значение в пиксели
      uint8_t py = gy + gh - 2 - (uint16_t)(v - lo) * (gh - 4) / (hi - lo);
      py = constrain(py, gy + 2, gy + gh - 3);

      if (prevPx >= 0) {
        oled.line(prevPx, prevPy, px, py);
      }
      oled.dot(px, py);

      // Отметки включения увлажнителя (точки внизу)
      if (humState & ((uint32_t)1 << dataIdx)) {
        oled.dot(px, gy + gh - 3);
        oled.dot(px, gy + gh - 4);
      }

      prevPx = px;
      prevPy = py;
    }
  }

  void drawMainScreen(float temp, float hum, uint8_t targetHum,
                      bool running, unsigned long workTime, bool sensorOK) {
    bool changed = false;
    if (firstDraw || sensorOK != lastSensorOK) changed = true;
    if (firstDraw || fabs(temp - lastTemp) >= 0.1) changed = true;
    if (firstDraw || fabs(hum - lastHum) >= 0.1) changed = true;
    if (firstDraw || targetHum != lastTargetHum) changed = true;
    if (firstDraw || running != lastRunning) changed = true;
    if (firstDraw || (workTime / 60) != (lastWorkTime / 60)) changed = true;

    if (changed) {
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

      oled.setCursor(70, 0);
      if (hum >= 0 && hum <= 100) {
        oled.print(hum, 1);
      } else {
        oled.print(F("--"));
      }
      oled.print(F("%"));

      // График
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

  void clear() { oled.clear(); oled.update(); }
  void update() { oled.update(); }
  void setCursor(uint8_t x, uint8_t y) { oled.setCursor(x, y); }
  void print(const char* t) { oled.print(t); }
  void print(const __FlashStringHelper* t) { oled.print(t); }
  void print(int v) { oled.print(v); }
  void print(float v, int d = 1) { oled.print(v, d); }
  void setScale(uint8_t s) { oled.setScale(s); }
  void drawLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) { oled.line(x0, y0, x1, y1); }

  void drawRect(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, bool fill = false) {
    if (fill) oled.rect(x0, y0, x1, y1, OLED_FILL);
    else {
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
