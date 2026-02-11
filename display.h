/*
 * МОДУЛЬ ДИСПЛЕЯ OLED 128x64
 * Отрисовка интерфейса с графиком влажности
 */

#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include <GyverOLED.h>
#include "config.h"

// Используем GyverOLED для работы с SSD1306
GyverOLED<SSD1306_128x64, OLED_BUFFER> oled;

// График влажности - уменьшено до 32 точек для экономии RAM
#define GRAPH_POINTS 32  // 32 точки = ~1 минута истории
#define GRAPH_HEIGHT 48  // увеличена высота графика

class Display {
private:
  // Кэш предыдущего состояния
  float lastTemp;
  float lastHum;
  uint8_t lastTargetHum;
  bool lastRunning;
  unsigned long lastWorkTime;
  bool lastSensorOK;
  bool firstDraw;

  // Данные графика - уменьшено до 32 точек
  uint8_t humidityGraph[GRAPH_POINTS];  // храним как uint8_t вместо float (4 байта -> 1 байт)
  uint8_t humidifierState;              // битовое хранение для 32 точек (32 байта -> 4 байта)
  uint8_t graphIndex;
  bool graphFilled;

public:
  Display() : lastTemp(NAN),
              lastHum(NAN),
              lastTargetHum(0),
              lastRunning(false),
              lastWorkTime(0),
              lastSensorOK(true),
              firstDraw(true),
              graphIndex(0),
              graphFilled(false),
              humidifierState(0) {
    // Инициализация графика нулями
    for (uint8_t i = 0; i < GRAPH_POINTS; i++) {
      humidityGraph[i] = 0;
    }
  }

  // Инициализация дисплея
  void begin() {
    oled.init();
    oled.clear();
    oled.setContrast(255);
  }

  // Заставка при загрузке
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

  // Добавление точки на график
  void addGraphPoint(float humidity, bool running) {
    // Сохраняем влажность как uint8_t (0-100)
    humidityGraph[graphIndex] = constrain((uint8_t)humidity, 0, 100);
    
    // Сохраняем состояние увлажнителя в битовом виде
    if (running) {
      bitSet(humidifierState, graphIndex);
    } else {
      bitClear(humidifierState, graphIndex);
    }
    
    graphIndex++;
    
    if (graphIndex >= GRAPH_POINTS) {
      graphIndex = 0;
      graphFilled = true;
    }
  }

  // Отрисовка графика влажности
  void drawHumidityGraph(uint8_t x, uint8_t y, uint8_t width, uint8_t height) {
    // Рамка графика
    oled.rect(x, y, x + width - 1, y + height - 1);
    
    uint8_t pointsToShow = graphFilled ? GRAPH_POINTS : graphIndex;
    
    // Если нет данных - просто пустой график
    if (pointsToShow == 0) {
      return;
    }

    // Находим min/max для масштабирования
    uint8_t minHum = 100, maxHum = 0;
    for (uint8_t i = 0; i < pointsToShow; i++) {
      uint8_t idx = graphFilled ? (graphIndex + i) % GRAPH_POINTS : i;
      uint8_t h = humidityGraph[idx];
      if (h < minHum) minHum = h;
      if (h > maxHum) maxHum = h;
    }

    // Добавляем отступы для красоты
    uint8_t range = maxHum - minHum;
    if (range < 10) range = 10;
    
    if (minHum > range / 10) minHum -= range / 10;
    else minHum = 0;
    
    if (maxHum + range / 10 <= 100) maxHum += range / 10;
    else maxHum = 100;

    // Рисуем график справа налево (новые данные справа)
    for (uint8_t i = 0; i < pointsToShow; i++) {
      uint8_t idx = graphFilled ? (graphIndex + i) % GRAPH_POINTS : i;
      uint8_t h = humidityGraph[idx];
      
      // Координата X справа налево
      uint8_t xPos = x + width - 2 - (i * (width - 2)) / GRAPH_POINTS;
      
      // Координата Y
      uint8_t yPos = y + height - 2 - ((h - minHum) * (height - 3)) / (maxHum - minHum);
      yPos = constrain(yPos, y + 1, y + height - 2);
      
      // Рисуем точку
      oled.dot(xPos, yPos);
      
      // Линия к предыдущей точке
      if (i > 0) {
        uint8_t prevIdx = graphFilled ? (graphIndex + i - 1) % GRAPH_POINTS : i - 1;
        uint8_t prevH = humidityGraph[prevIdx];
        uint8_t prevX = x + width - 2 - ((i - 1) * (width - 2)) / GRAPH_POINTS;
        uint8_t prevY = y + height - 2 - ((prevH - minHum) * (height - 3)) / (maxHum - minHum);
        prevY = constrain(prevY, y + 1, y + height - 2);
        
        oled.line(prevX, prevY, xPos, yPos);
      }
      
      // Отметки включения увлажнителя
      if (bitRead(humidifierState, idx)) {
        oled.dot(xPos, y + height - 3);
        oled.dot(xPos, y + height - 4);
      }
    }
  }

  // Главный экран
  void drawMainScreen(float temp, float hum, uint8_t targetHum, bool running, unsigned long workTime, bool sensorOK) {
    // Проверяем, что изменилось
    bool changed = false;

    if (firstDraw || sensorOK != lastSensorOK) changed = true;
    if (firstDraw || fabs(temp - lastTemp) >= 0.1) changed = true;
    if (firstDraw || fabs(hum - lastHum) >= 0.1) changed = true;
    if (firstDraw || targetHum != lastTargetHum) changed = true;
    if (firstDraw || running != lastRunning) changed = true;
    if (firstDraw || (workTime / 60) != (lastWorkTime / 60)) changed = true;

    if (changed) {
      oled.clear();

      // Проверка ошибки датчика
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

      // Температура и влажность в одну строку
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

      // График влажности внизу (занимает оставшуюся часть экрана)
      drawHumidityGraph(0, 16, 128, GRAPH_HEIGHT);

      oled.update();

      // Обновляем кэш
      lastTemp = temp;
      lastHum = hum;
      lastTargetHum = targetHum;
      lastRunning = running;
      lastWorkTime = workTime;
      lastSensorOK = sensorOK;
      firstDraw = false;
    }
  }

  // Экран "О системе"
  void drawAboutScreen(unsigned long workTime, uint8_t switchCount, unsigned long totalSwitches) {
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
  void drawCalibrationScreen(float currentTemp, float currentHum, float tempCal, float humCal, bool editingTemp) {
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

  // Очистка экрана
  void clear() {
    oled.clear();
    oled.update();
  }

  // Обновление экрана
  void update() {
    oled.update();
  }

  // Установка курсора
  void setCursor(uint8_t x, uint8_t y) {
    oled.setCursor(x, y);
  }

  // Печать текста
  void print(const char* text) {
    oled.print(text);
  }

  void print(const __FlashStringHelper* text) {
    oled.print(text);
  }

  void print(int value) {
    oled.print(value);
  }

  void print(float value, int decimals = 1) {
    oled.print(value, decimals);
  }

  // Установка масштаба шрифта
  void setScale(uint8_t scale) {
    oled.setScale(scale);
  }

  // Рисование линии
  void drawLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
    oled.line(x0, y0, x1, y1);
  }

  // Рисование прямоугольника
  void drawRect(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, bool fill = false) {
    if (fill) {
      oled.rect(x0, y0, x1, y1, OLED_FILL);
    } else {
      oled.rect(x0, y0, x1, y1);
    }
  }

  // Инвертирование области
  void invertRect(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
    oled.rect(x0, y0, x1, y1, OLED_FILL);
  }
};

#endif // DISPLAY_H
