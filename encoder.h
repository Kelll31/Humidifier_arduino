/*
 * МОДУЛЬ ЭНКОДЕРА
 * Обработка вращения и нажатий с использованием библиотеки GyverEncoder
 */

#ifndef ENCODER_H
#define ENCODER_H

#include <Arduino.h>
#include <GyverEncoder.h>
#include "config.h"

class EncoderModule {
private:
  Encoder enc;
  
public:
  EncoderModule() : enc(ENCODER_CLK, ENCODER_DT, ENCODER_SW) {}

  // Инициализация энкодера
  void begin() {
    enc.setType(TYPE2);  // Тип энкодера (TYPE1 или TYPE2 в зависимости от модели)
  }

  // Опрос энкодера (вызывать в loop)
  void tick() {
    enc.tick();
  }

  // Получение изменения позиции (-1, 0, +1)
  int8_t getDelta() {
    if (enc.isRight()) {
      return 1;
    }
    if (enc.isLeft()) {
      return -1;
    }
    return 0;
  }

  // Проверка короткого нажатия
  bool isClick() {
    return enc.isClick();
  }

  // Проверка двойного клика
  bool isDouble() {
    return enc.isDouble();
  }

  // Проверка длинного нажатия (удержание)
  bool isLongPress() {
    return enc.isHolded();
  }

  // Очистка флага длинного нажатия (не требуется с GyverEncoder)
  void clearLongPress() {
    // GyverEncoder автоматически сбрасывает флаги
  }

  // Проверка быстрого вращения
  bool isFastRotate() {
    return enc.isFast();
  }

  // Проверка нажатого вращения вправо
  bool isTurnRightH() {
    return enc.isRightH();
  }

  // Проверка нажатого вращения влево
  bool isTurnLeftH() {
    return enc.isLeftH();
  }

  // Проверка вращения в любую сторону
  bool isTurn() {
    return enc.isTurn();
  }

  // Сброс позиции (не применимо к GyverEncoder)
  void resetPosition() {
    // GyverEncoder не хранит абсолютную позицию
  }

  // Получение нажатия кнопки (состояние)
  bool isPressed() {
    return enc.isHold();
  }

  // Проверка одиночного клика (с таймаутом для двойного)
  bool isSingle() {
    return enc.isSingle();
  }

  // Проверка отпускания кнопки
  bool isRelease() {
    return enc.isRelease();
  }

  // Проверка нажатия (с дебаунсом)
  bool isPress() {
    return enc.isPress();
  }
};

#endif // ENCODER_H
