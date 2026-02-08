/*
 * МОДУЛЬ ДИСПЛЕЯ OLED 128x64
 * Отрисовка интерфейса
 */

#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include <GyverOLED.h>
#include "config.h"

// Используем GyverOLED для работы с SSD1306
GyverOLED<SSD1306_128x64, OLED_BUFFER> oled;

class Display {
private:
  unsigned long lastUpdateTime;
  bool needUpdate;

public:
  Display() : lastUpdateTime(0), needUpdate(true) {}

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

  // Главный экран
  void drawMainScreen(float temp, float hum, uint8_t targetHum, bool running, unsigned long workTime) {
    oled.clear();

    // Заголовок
    oled.setScale(1);
    oled.setCursor(25, 0);
    oled.print(F("УВЛАЖНИТЕЛЬ"));

    // Разделительная линия
    oled.line(0, 12, 127, 12);

    // Температура
    oled.setCursor(0, 2);
    oled.print(F("Темп: "));
    oled.setScale(2);
    oled.setCursor(40, 2);
    if (temp >= 0) oled.print(temp, 1);
    else oled.print(F("--"));
    oled.setScale(1);
    oled.print(F("C"));

    // Влажность
    oled.setCursor(0, 4);
    oled.print(F("Влажн:"));
    oled.setScale(2);
    oled.setCursor(40, 4);
    if (hum >= 0) oled.print(hum, 1);
    else oled.print(F("--"));
    oled.setScale(1);
    oled.print(F("%"));

    // Прогресс-бар влажности
    drawProgressBar(0, 52, 127, 8, hum, targetHum);

    // Статус увлажнителя
    oled.setCursor(0, 7);
    oled.print(F("Цель:"));
    oled.print(targetHum);
    oled.print(F("%"));

    oled.setCursor(65, 7);
    if (running) {
      oled.print(F("ВКЛ"));
      // Анимация работы
      uint8_t anim = (millis() / 500) % 3;
      for (uint8_t i = 0; i <= anim; i++) {
        oled.print(F("."));
      }
    } else {
      oled.print(F("ВЫКЛ"));
    }

    oled.update();
  }

  // Прогресс-бар
  void drawProgressBar(uint8_t x, uint8_t y, uint8_t width, uint8_t height, float value, float maxValue) {
    // Рамка
    oled.rect(x, y, x + width, y + height);

    // Заполнение
    uint8_t fillWidth = (uint8_t)((value / maxValue) * (width - 2));
    fillWidth = constrain(fillWidth, 0, width - 2);

    if (fillWidth > 0) {
      oled.rect(x + 1, y + 1, x + 1 + fillWidth, y + height - 1, OLED_FILL);
    }
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
