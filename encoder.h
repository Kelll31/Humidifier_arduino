/*
 * МОДУЛЬ ЭНКОДЕРА
 * Обработка вращения и нажатий
 */

#ifndef ENCODER_H
#define ENCODER_H

#include <Arduino.h>
#include "config.h"

class EncoderModule {
private:
  int8_t position;
  int8_t lastPosition;
  bool buttonState;
  bool lastButtonState;
  unsigned long buttonPressTime;
  unsigned long lastRotateTime;
  bool longPressDetected;

  uint8_t clkState;
  uint8_t dtState;
  uint8_t lastClkState;

public:
  EncoderModule() : position(0), 
                    lastPosition(0),
                    buttonState(HIGH),
                    lastButtonState(HIGH),
                    buttonPressTime(0),
                    lastRotateTime(0),
                    longPressDetected(false),
                    clkState(HIGH),
                    dtState(HIGH),
                    lastClkState(HIGH) {}

  // Инициализация энкодера
  void begin() {
    pinMode(ENCODER_CLK, INPUT_PULLUP);
    pinMode(ENCODER_DT, INPUT_PULLUP);
    pinMode(ENCODER_SW, INPUT_PULLUP);

    lastClkState = digitalRead(ENCODER_CLK);
  }

  // Обработка энкодера (вызывать в loop)
  void tick() {
    // Чтение состояния CLK
    clkState = digitalRead(ENCODER_CLK);

    // Обнаружение вращения
    if (clkState != lastClkState && clkState == LOW) {
      dtState = digitalRead(ENCODER_DT);

      // Определение направления
      if (dtState != clkState) {
        position++; // Поворот по часовой стрелке
      } else {
        position--; // Поворот против часовой стрелки
      }

      lastRotateTime = millis();
    }

    lastClkState = clkState;

    // Обработка кнопки
    bool currentButtonState = digitalRead(ENCODER_SW);

    if (currentButtonState != lastButtonState) {
      delay(10); // Дебаунсинг
      currentButtonState = digitalRead(ENCODER_SW);

      if (currentButtonState == LOW && lastButtonState == HIGH) {
        // Кнопка нажата
        buttonPressTime = millis();
        longPressDetected = false;
      }
    }

    // Обнаружение длинного нажатия
    if (currentButtonState == LOW && !longPressDetected) {
      if (millis() - buttonPressTime >= LONG_PRESS_TIME) {
        longPressDetected = true;
      }
    }

    buttonState = currentButtonState;
    lastButtonState = currentButtonState;
  }

  // Проверка изменения позиции
  bool isChanged() {
    if (position != lastPosition) {
      lastPosition = position;
      return true;
    }
    return false;
  }

  // Получение изменения позиции
  int8_t getDelta() {
    int8_t delta = position - lastPosition;
    lastPosition = position;
    return delta;
  }

  // Получение текущей позиции
  int8_t getPosition() const {
    return position;
  }

  // Сброс позиции
  void resetPosition() {
    position = 0;
    lastPosition = 0;
  }

  // Проверка короткого нажатия
  bool isClick() {
    if (buttonState == HIGH && lastButtonState == LOW) {
      unsigned long pressDuration = millis() - buttonPressTime;
      if (pressDuration < LONG_PRESS_TIME && !longPressDetected) {
        return true;
      }
    }
    return false;
  }

  // Проверка длинного нажатия
  bool isLongPress() {
    return longPressDetected;
  }

  // Проверка быстрого вращения
  bool isFastRotate() const {
    return (millis() - lastRotateTime) < ENCODER_FAST_THRESHOLD;
  }

  // Очистка флага длинного нажатия
  void clearLongPress() {
    longPressDetected = false;
  }
};

#endif // ENCODER_H
