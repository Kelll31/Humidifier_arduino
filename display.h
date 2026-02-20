/*
 * МОДУЛЬ ДИСПЛЕЯ OLED 128x64
 * Использует библиотеку GyverOLED (режим без буфера)
 * Поддержка русского языка
 * v2.2 - исправление черного экрана
 */

#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include <Wire.h>
#include "config.h"

// Подключаем GyverOLED напрямую из локальной библиотеки
#include <GyverOLED.h>

enum DisplayMode
{
  MODE_DATA = 0,
  MODE_GRAPH = 1
};

// Экраны внутри режима графика
enum GraphScreen
{
  GRAPH_SCREEN_GRAPH = 0,
  GRAPH_SCREEN_STATS = 1
};

#define GRAPH_POINTS 32

class Display
{
private:
  // OLED объект как член класса (предотвращает multiple definition)
  GyverOLED<SSD1306_128x64, OLED_NO_BUFFER, OLED_I2C> oled;
  
  int cursorX, cursorY;
  uint8_t textScale;
  bool invert;

  float lastTemp, lastHum;
  uint8_t lastTargetHum;
  bool lastRunning;
  unsigned long lastWorkTime;
  bool lastSensorOK;
  bool lastWaterLow;
  bool firstDraw;
  int lastWaterValue;
  bool lastWaterPresent;

  uint8_t currentBrightness;
  DisplayMode currentMode;
  uint8_t graphScreen; // 0 = график, 1 = статистика

  uint8_t humGraph[GRAPH_POINTS];
  uint32_t humState;
  uint8_t gIdx;
  bool gFull;

  uint8_t lastChar;

public:
  Display() : cursorX(0), cursorY(0), textScale(1), invert(false),
              lastTemp(-999), lastHum(-999), lastTargetHum(0),
              lastRunning(false), lastWorkTime(0), lastSensorOK(true),
              lastWaterLow(false), firstDraw(true), lastWaterValue(0),
              lastWaterPresent(false), currentBrightness(BRIGHTNESS_FULL),
              currentMode(MODE_DATA), graphScreen(GRAPH_SCREEN_GRAPH),
              gIdx(0), gFull(false), humState(0),
              lastChar(0)
  {
    memset(humGraph, 0, sizeof(humGraph));
  }

  void begin()
  {
    // Инициализация I2C шины
    Wire.begin();
    Wire.setClock(400000L); // 400 kHz Fast Mode
    
    // Задержка для стабилизации I2C
    delay(100);
    
    // Инициализация OLED
    oled.init(OLED_ADDRESS);
    delay(50);
    
    oled.clear();
    oled.update();
    delay(50);
    
    setBrightness(BRIGHTNESS_FULL);
  }

  void setBrightness(uint8_t brightness)
  {
    if (currentBrightness != brightness)
    {
      currentBrightness = brightness;
      oled.setContrast(brightness);
    }
  }

  uint8_t getBrightness() const { return currentBrightness; }

  void toggleMode()
  {
    currentMode = (currentMode == MODE_DATA) ? MODE_GRAPH : MODE_DATA;
    graphScreen = GRAPH_SCREEN_GRAPH; // Сбрасываем на первый экран
    firstDraw = true;
  }

  // Переключение между экранами внутри режима графика
  void toggleGraphScreen()
  {
    graphScreen = (graphScreen == GRAPH_SCREEN_GRAPH) ? GRAPH_SCREEN_STATS : GRAPH_SCREEN_GRAPH;
    firstDraw = true;
  }

  uint8_t getGraphScreen() const { return graphScreen; }

  DisplayMode getMode() const { return currentMode; }

  void invertText(bool inv)
  {
    invert = inv;
    oled.invertText(inv);
  }

  void textMode(byte m) { oled.textMode(m); }

  void showSplash()
  {
    oled.clear();
    cursorX = 0;
    cursorY = 16;
    oled.setCursor(cursorX, cursorY);
    oled.setScale(2);
    oled.print("УВЛАЖНИТЕЛЬ");
    oled.update();
    delay(1000);
    
    cursorX = 30;
    cursorY = 48;
    oled.setCursor(cursorX, cursorY);
    oled.setScale(1);
    oled.print("v");
    oled.print(FIRMWARE_VERSION);
    oled.update();
    delay(500);
  }

  void addGraphPoint(float humidity, bool running)
  {
    uint8_t val = (uint8_t)constrain(humidity, 0, 100);
    humGraph[gIdx] = val;
    if (running)
      humState |= ((uint32_t)1 << gIdx);
    else
      humState &= ~((uint32_t)1 << gIdx);
    gIdx++;
    if (gIdx >= GRAPH_POINTS)
    {
      gIdx = 0;
      gFull = true;
    }
  }

  void dot(int x, int y, byte fill = 1)
  {
    oled.dot(x, y, fill);
  }

  void drawLine(int x0, int y0, int x1, int y1, byte fill = 1)
  {
    oled.line(x0, y0, x1, y1, fill);
  }

  void line(int x0, int y0, int x1, int y1, byte fill = 1)
  {
    oled.line(x0, y0, x1, y1, fill);
  }

  void fastLineH(int y, int x0, int x1, byte fill = 1)
  {
    oled.fastLineH(y, x0, x1, fill);
  }

  void fastLineV(int x, int y0, int y1, byte fill = 1)
  {
    oled.fastLineV(x, y0, y1, fill);
  }

  void drawGraph()
  {
    line(0, 20, 127, 20);
    line(0, 63, 127, 63);
    line(0, 20, 0, 63);
    line(127, 20, 127, 63);

    uint8_t cnt = gFull ? GRAPH_POINTS : gIdx;
    if (cnt < 2)
      return;

    int16_t prevX = -1, prevY = -1;

    for (uint8_t i = 0; i < cnt; i++)
    {
      uint8_t idx = gFull ? ((gIdx + i) % GRAPH_POINTS) : i;
      uint8_t v = humGraph[idx];

      uint8_t px = 126 - (uint16_t)(cnt - 1 - i) * 124 / (cnt - 1);
      uint8_t py = 61 - (uint16_t)v * 40 / 100;
      py = constrain(py, 22, 61);

      if (prevX >= 0)
        line(prevX, prevY, px, py);
      dot(px, py);

      if (humState & ((uint32_t)1 << idx))
        dot(px, 61);

      prevX = px;
      prevY = py;
    }
  }

  // Экран статистики
  void drawStatsScreen(float temp, float hum, bool running, unsigned long workTime,
                       bool waterLow, bool waterSensorPresent, uint8_t waterPercent)
  {
    oled.clear();
    
    // Заголовок
    cursorX = 30;
    cursorY = 0;
    oled.setCursor(cursorX, cursorY);
    oled.setScale(1);
    oled.print("СТАТИСТИКА");
    line(0, 1, 127, 1);

    // Температура
    cursorX = 0;
    cursorY = 2;
    oled.setCursor(cursorX, cursorY);
    oled.print("T:");
    if (temp >= -40 && temp <= 80)
      oled.print((int)temp);
    else
      oled.print("--");
    oled.print("C");

    // Влажность
    cursorX = 60;
    cursorY = 2;
    oled.setCursor(cursorX, cursorY);
    oled.print("H:");
    if (hum >= 0 && hum <= 100)
      oled.print((int)hum);
    else
      oled.print("--");
    oled.print("%");

    // Статус увлажнителя
    cursorX = 0;
    cursorY = 3;
    oled.setCursor(cursorX, cursorY);
    oled.print("Увлажнитель:");
    if (running)
      oled.print("ВКЛ");
    else
      oled.print("ВЫКЛ");

    // Время работы
    cursorX = 0;
    cursorY = 4;
    oled.setCursor(cursorX, cursorY);
    oled.print("Работа:");
    if (workTime >= 3600) {
      oled.print(workTime / 3600);
      oled.print("ч");
    }
    oled.print((workTime % 3600) / 60);
    oled.print("м");

    // Уровень воды
    cursorX = 0;
    cursorY = 5;
    oled.setCursor(cursorX, cursorY);
    oled.print("Вода:");
    if (!waterSensorPresent)
      oled.print("НЕТ");
    else if (waterLow)
      oled.print("НИЗКО!");
    else {
      oled.print(waterPercent);
      oled.print("%");
    }

    // Подсказка
    cursorX = 0;
    cursorY = 7;
    oled.setCursor(cursorX, cursorY);
    oled.print("Поворот-выбор");

    oled.update();
  }

  void drawMainScreen(float temp, float hum, uint8_t targetHum,
                      bool running, unsigned long workTime, bool sensorOK,
                      bool waterLow, bool windowOpen,
                      bool waterSensorPresent, uint8_t waterPercent,
                      int waterRawValue)
  {
    // Проверяем, нужно ли перерисовать
    bool needRedraw = firstDraw;
    
    if (currentMode == MODE_GRAPH && graphScreen == GRAPH_SCREEN_STATS) {
      // Для экрана статистики - всегда перерисовываем
      needRedraw = true;
    }
    
    if (sensorOK != lastSensorOK) needRedraw = true;
    if (waterLow != lastWaterLow) needRedraw = true;
    if (fabs(temp - lastTemp) >= 0.5) needRedraw = true;
    if (fabs(hum - lastHum) >= 1) needRedraw = true;
    if (targetHum != lastTargetHum) needRedraw = true;
    if (running != lastRunning) needRedraw = true;

    if (!needRedraw && !firstDraw) return;

    // Если режим графика и экран статистики - рисуем статистику
    if (currentMode == MODE_GRAPH && graphScreen == GRAPH_SCREEN_STATS) {
      drawStatsScreen(temp, hum, running, workTime, waterLow, waterSensorPresent, waterPercent);
    } else {
      // Иначе рисуем основной экран или график
      drawDataScreen(temp, hum, targetHum, running, workTime, sensorOK,
                     waterLow, waterSensorPresent, waterPercent, waterRawValue);
    }

    lastTemp = temp;
    lastHum = hum;
    lastTargetHum = targetHum;
    lastRunning = running;
    lastWorkTime = workTime;
    lastSensorOK = sensorOK;
    lastWaterLow = waterLow;
    lastWaterPresent = waterSensorPresent;
    lastWaterValue = waterRawValue;
    firstDraw = false;
  }

  // Основной экран с данными и графиком
  void drawDataScreen(float temp, float hum, uint8_t targetHum,
                      bool running, unsigned long workTime, bool sensorOK,
                      bool waterLow, bool waterSensorPresent, uint8_t waterPercent,
                      int waterRawValue)
  {
    oled.clear();

    if (!sensorOK)
    {
      oled.setCursor(15, 3);
      oled.setScale(2);
      oled.print("ОШИБКА");
      oled.setCursor(20, 6);
      oled.setScale(1);
      oled.print("DHT22");
      lastSensorOK = sensorOK;
      firstDraw = false;
      oled.update();
      return;
    }

    cursorX = 0;
    cursorY = 0;
    oled.setCursor(cursorX, cursorY);
    oled.setScale(2);
    if (temp >= -40 && temp <= 80)
      oled.print((int)temp);
    else
      oled.print("--");
    oled.print("C");

    cursorX = 70;
    cursorY = 0;
    oled.setCursor(cursorX, cursorY);
    if (hum >= 0 && hum <= 100)
      oled.print((int)hum);
    else
      oled.print("--");
    oled.print("%");

    cursorX = 0;
    cursorY = 2;
    oled.setCursor(cursorX, cursorY);
    oled.setScale(1);
    oled.print("SET:");
    oled.print(targetHum);
    oled.print("%");

    cursorX = 55;
    cursorY = 2;
    oled.setCursor(cursorX, cursorY);
    if (!waterSensorPresent)
      oled.print("--");
    else if (waterLow)
      oled.print("NO WATER!");
    else if (waterPercent < 30)
      oled.print("LOW");
    else
    {
      oled.print("OK");
      oled.print(waterPercent);
      oled.print("%");
    }

    if (currentMode == MODE_GRAPH) {
      drawGraph();
    }

    oled.update();
  }

  void drawAboutScreen(unsigned long workTime, uint8_t switchCount,
                       unsigned long totalSwitches, bool waterSensorPresent,
                       uint16_t waterThreshold, int waterRawValue)
  {
    oled.clear();
    cursorX = 20;
    cursorY = 0;
    oled.setCursor(cursorX, cursorY);
    oled.setScale(1);
    oled.print("О СИСТЕМЕ");
    line(0, 1, 127, 1);

    cursorX = 0;
    cursorY = 2;
    oled.setCursor(cursorX, cursorY);
    oled.print("v");
    oled.print(FIRMWARE_VERSION);
    cursorX = 70;
    cursorY = 2;
    oled.setCursor(cursorX, cursorY);
    oled.print("kelll31");

    cursorX = 0;
    cursorY = 3;
    oled.setCursor(cursorX, cursorY);
    oled.print("Работа:");
    oled.print(workTime / 3600);
    oled.print("ч");
    oled.print((workTime % 3600) / 60);
    oled.print("м");

    cursorX = 0;
    cursorY = 4;
    oled.setCursor(cursorX, cursorY);
    oled.print("Перекл:");
    oled.print(switchCount);
    oled.print("/ч");

    if (waterSensorPresent)
    {
      cursorX = 0;
      cursorY = 5;
      oled.setCursor(cursorX, cursorY);
      oled.print("Вода:");
      oled.print(waterRawValue);
      oled.print("/");
      oled.print(waterThreshold);
    }

    cursorX = 0;
    cursorY = 7;
    oled.setCursor(cursorX, cursorY);
    oled.print("ДЛ-выход");
    oled.update();
  }

  void drawCalibrationScreen(float currentTemp, float currentHum,
                             float tempCal, float humCal, bool editingTemp)
  {
    oled.clear();
    cursorX = 15;
    cursorY = 0;
    oled.setCursor(cursorX, cursorY);
    oled.setScale(1);
    oled.print("КАЛИБРОВКА");
    line(0, 1, 127, 1);

    cursorX = 0;
    cursorY = 2;
    oled.setCursor(cursorX, cursorY);
    oled.print("Т:");
    oled.print((int)currentTemp);
    oled.print("C В:");
    oled.print((int)currentHum);
    oled.print("%");

    cursorX = 0;
    cursorY = 4;
    oled.setCursor(cursorX, cursorY);
    if (editingTemp)
      oled.print("> ");
    oled.print("КорТ:");
    if (tempCal >= 0)
      oled.print("+");
    oled.print((int)tempCal);

    cursorX = 0;
    cursorY = 5;
    oled.setCursor(cursorX, cursorY);
    if (!editingTemp)
      oled.print("> ");
    oled.print("КорВ:");
    if (humCal >= 0)
      oled.print("+");
    oled.print((int)humCal);

    cursorX = 0;
    cursorY = 7;
    oled.setCursor(cursorX, cursorY);
    oled.print("КН- след ДЛ-OK");
    oled.update();
  }

  void drawWaterCalibrationScreen(int currentValue, uint16_t threshold,
                                  bool sensorPresent, uint8_t waterPercent)
  {
    oled.clear();
    cursorX = 10;
    cursorY = 0;
    oled.setCursor(cursorX, cursorY);
    oled.setScale(1);
    oled.print("ПОРОГ ВОДЫ");
    line(0, 1, 127, 1);

    cursorX = 0;
    cursorY = 2;
    oled.setCursor(cursorX, cursorY);
    if (!sensorPresent)
      oled.print("НЕТ ДАТЧИКА!");
    else
    {
      oled.print("Текущий:");
      oled.print(currentValue);
      cursorX = 0;
      cursorY = 3;
      oled.setCursor(cursorX, cursorY);
      oled.print("Порог:");
      oled.print(threshold);
      if ((uint16_t)currentValue < threshold)
      {
        cursorX = 0;
        cursorY = 4;
        oled.setCursor(cursorX, cursorY);
        oled.print("НИЗКАЯ ВОДА!");
      }
      cursorX = 0;
      cursorY = 5;
      oled.setCursor(cursorX, cursorY);
      oled.print("Уровень:");
      oled.print(waterPercent);
      oled.print("%");
    }

    cursorX = 0;
    cursorY = 7;
    oled.setCursor(cursorX, cursorY);
    oled.print("КН-меню ДЛ-сохр");
    oled.update();
  }

  // Экран ручного режима
  void drawManualScreen(bool isOn)
  {
    oled.clear();
    cursorX = 25;
    cursorY = 0;
    oled.setCursor(cursorX, cursorY);
    oled.setScale(1);
    oled.print("РУЧНОЙ РЕЖИМ");
    line(0, 1, 127, 1);

    cursorX = 20;
    cursorY = 3;
    oled.setCursor(cursorX, cursorY);
    oled.setScale(2);
    if (isOn) {
      oled.print("ВКЛ");
    } else {
      oled.print("ВЫКЛ");
    }

    cursorX = 0;
    cursorY = 6;
    oled.setCursor(cursorX, cursorY);
    oled.setScale(1);
    oled.print("Поворот:перекл");

    cursorX = 0;
    cursorY = 7;
    oled.setCursor(cursorX, cursorY);
    oled.print("КН-выход");

    oled.update();
  }

  void clear()
  {
    oled.clear();
  }

  void update()
  {
    oled.update();
  }

  void setCursor(uint8_t x, uint8_t y)
  {
    cursorX = x;
    cursorY = y;
    oled.setCursor(x, y);
  }

  void print(const char *t) { oled.print(t); }
  void print(const __FlashStringHelper *t) { oled.print(t); }
  void print(int v) { oled.print(v); }
  void print(float v, int d = 1) { oled.print((int)v); }
  void setScale(uint8_t s)
  {
    textScale = constrain(s, 1, 4);
    oled.setScale(textScale);
  }

  void drawRect(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, bool fill = false)
  {
    oled.rect(x0, y0, x1, y1, fill ? OLED_FILL : OLED_STROKE);
  }

  void invertRect(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
  {
  oled.rect(x0, y0, x1, y1, OLED_FILL);
  }
};

#endif // DISPLAY_H
