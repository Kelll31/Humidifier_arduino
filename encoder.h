/*
 * МОДУЛЬ ЭНКОДЕРА
 * Обработка вращения и нажатий с использованием библиотеки EncButton
 * Современная замена устаревшей GyverEncoder
 */

#ifndef ENCODER_H
#define ENCODER_H

#include <Arduino.h>
#include <EncButton.h>
#include "config.h"

class EncoderModule {
private:
  EncButton<EB_TICK, ENCODER_DT, ENCODER_CLK, ENCODER_SW> enc;
  
public:
  EncoderModule() {}

  // Инициализация энкодера
  void begin() {
    enc.setEncType(EB_STEP4_LOW);  // Тип энкодера (активный низкий сигнал)
  }

  // Опрос энкодера (вызывать в loop)
  void tick() {
    enc.tick();
  }

  // Получение изменения позиции (-1, 0, +1)
  int8_t getDelta() {
    if (enc.turn()) {
      return enc.dir();
    }
    return 0;
  }

  // Проверка короткого нажатия
  bool isClick() {
    return enc.click();
  }

  // Проверка двойного клика
  bool isDouble() {
    return enc.hasClicks(2);
  }

  // Проверка длинного нажатия
  bool isLongPress() {
    return enc.hold();
  }

  // Очистка флага длинного нажатия (не требуется с EncButton)
  void clearLongPress() {
    // EncButton автоматически сбрасывает флаги
  }

  // Проверка быстрого вращения
  bool isFastRotate() {
    return enc.fast();
  }

  // Проверка нажатого вращения
  bool isTurnH() {
    return enc.turnH();
  }

  // Сброс позиции (не применимо к EncButton)
  void resetPosition() {
    // EncButton не хранит абсолютную позицию
  }

  // Получение нажатия кнопки (состояние)
  bool isPressed() {
    return enc.pressing();
  }

  // Проверка одиночного клика
  bool isSingle() {
    return enc.click();
  }
};

#endif // ENCODER_H
