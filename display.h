/*
 * МОДУЛЬ ДИСПЛЕЯ OLED 128x64
 * Отрисовка интерфейса с минимальным количеством перерисовок
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
  // Кэш предыдущего состояния для уменьшения мерцания
  float lastTemp;
  float lastHum;
  uint8_t lastTargetHum;
  bool lastRunning;
  unsigned long lastWorkTime;
  bool lastSensorOK;
  bool firstDraw;

public:
  Display() : lastTemp(NAN),
              lastHum(NAN),
              lastTargetHum(0),
              lastRunning(false),
              lastWorkTime(0),
              lastSensorOK(true),
              firstDraw(true) {}

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
  // Теперь перерисовываем только при изменении данных, чтобы убрать полосы
  void drawMainScreen(float temp, float hum, uint8_t targetHum, bool running, unsigned long workTime, bool sensorOK) {
    // Если данные не изменились и это не первый кадр - просто анимируем статус
    bool changed = false;

    if (firstDraw || sensorOK != lastSensorOK) changed = true;
    if (firstDraw || fabs(temp - lastTemp) >= 0.1) changed = true;
    if (firstDraw || fabs(hum - lastHum) >= 0.1) changed = true;
    if (firstDraw || targetHum != lastTargetHum) changed = true;
    if (firstDraw || running != lastRunning) changed = true;
    if (firstDraw || (workTime / 60) != (lastWorkTime / 60)) changed = true; // обновляем раз в минуту

    if (changed) {
      oled.clear();

      // Заголовок
      oled.setScale(1);
      oled.setCursor(25, 0);
      oled.print(F("УВЛАЖНИТЕЛЬ"));

      // Разделительная линия
      oled.line(0, 12, 127, 12);

      // Проверка ошибки датчика
      if (!sensorOK) {
        oled.setScale(2);
        oled.setCursor(15, 3);
        oled.print(F("ОШИБКА"));
        oled.setScale(1);
        oled.setCursor(20, 6);
        oled.print(F("Датчик DHT22"));
        oled.update();

        // Обновляем кэш
        lastSensorOK = sensorOK;
        firstDraw = false;
        return;
      }

      // Температура
      oled.setCursor(0, 2);
      oled.print(F("Темп:"));
      oled.setScale(2);
      oled.setCursor(40, 2);
      if (temp >= -40 && temp <= 80) {
        oled.print(temp, 1);
      } else {
        oled.print(F("--"));
      }
      oled.setScale(1);
      oled.print(F("C"));

      // Влажность
      oled.setCursor(0, 4);
      oled.print(F("Влажн:"));
      oled.setScale(2);
      oled.setCursor(40, 4);
      if (hum >= 0 && hum <= 100) {
        oled.print(hum, 1);
      } else {
        oled.print(F("--"));
      }
      oled.setScale(1);
      oled.print(F("%"));

      // Прогресс-бар влажности (график снизу)
      drawProgressBar(0, 52, 127, 8, hum, targetHum);

      // Статус увлажнителя
      oled.setCursor(0, 7);
      oled.print(F("Цель:"));
      oled.print(targetHum);
      oled.print(F("%"));

      // Время работы (часы)
      if (workTime >= 3600) {
        oled.setCursor(50, 7);
        oled.print(workTime / 3600);
        oled.print(F("ч"));
      }

      // Статусный текст, анимацию добавим ниже отдельно
      oled.setCursor(85, 7);
      if (running) {
        oled.print(F("ВКЛ"));
      } else {
        oled.print(F("ВЫКЛ"));
      }

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

    // Отдельно дорисовываем анимацию без полного обновления экрана
    if (sensorOK) {
      oled.setScale(1);
      oled.setCursor(88, 7);
      if (running) {
        uint8_t anim = (millis() / 500) % 4; // 0..3
        for (uint8_t i = 0; i < 3; i++) {
          if (i < anim) oled.print(F("."));
          else oled.print(F(" "));
        }
      } else {
        oled.print(F("   "));
      }
      oled.update();
    }
  }

  // Прогресс-бар (график влажности)
  void drawProgressBar(uint8_t x, uint8_t y, uint8_t width, uint8_t height, float value, float maxValue) {
    // Защита от деления на ноль и некорректных значений
    if (maxValue < 1) maxValue = 1;
    if (value < 0) value = 0;
    if (value > 100) value = 100;

    // Рамка
    oled.rect(x, y, x + width, y + height);

    // Заполнение: % от 0 до 100 в ширину бара
    uint8_t fillWidth = (uint8_t)((value / 100.0) * (width - 2));
    fillWidth = constrain(fillWidth, 0, width - 2);

    if (fillWidth > 0) {
      oled.rect(x + 1, y + 1, x + 1 + fillWidth, y + height - 1, OLED_FILL);
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

    // Время работы
    oled.setCursor(0, 4);
    oled.print(F("Работа: "));
    if (workTime >= 3600) {
      oled.print(workTime / 3600);
      oled.print(F("ч "));
    }
    oled.print((workTime % 3600) / 60);
    oled.print(F("м"));

    // Количество переключений в час
    oled.setCursor(0, 5);
    oled.print(F("Перекл:"));
    oled.print(switchCount);
    oled.print(F("/час"));

    // Общее количество переключений
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

    // Текущие значения
    oled.setCursor(0, 2);
    oled.print(F("Т: "));
    oled.print(currentTemp, 1);
    oled.print(F("C  В: "));
    oled.print(currentHum, 1);
    oled.print(F("%"));

    // Коррекция температуры
    oled.setCursor(0, 4);
    if (editingTemp) oled.print(F("> "));
    oled.print(F("Корр.Т: "));
    if (tempCal >= 0) oled.print(F("+"));
    oled.print(tempCal, 1);
    oled.print(F("C"));

    // Коррекция влажности
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
