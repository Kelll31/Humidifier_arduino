/*
 * МОДУЛЬ ЭНКОДЕРА
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

  void begin() {
    enc.setType(TYPE2);
  }

  void tick() {
    enc.tick();
  }

  // Проверка - было ли вращение (любое направление)
  bool isTurn() {
    return enc.isTurn();
  }

  // Возвращает: 1 = вправо, -1 = влево, 0 = нет вращения
  int8_t getDelta() {
    if (enc.isRight()) return 1;
    if (enc.isLeft()) return -1;
    return 0;
  }

  int8_t getDirection() {
    return getDelta();
  }

  bool isClick() { return enc.isClick(); }
  bool isDouble() { return enc.isDouble(); }
  bool isLongPress() { return enc.isHolded(); }
  void clearLongPress() {}

  bool isFastRotate() { return enc.isFastR() || enc.isFastL(); }

  bool isRight() { return enc.isRight(); }
  bool isLeft() { return enc.isLeft(); }

  void resetPosition() {}

  bool isPressed() { return enc.isHold(); }
  bool isPress() { return enc.isPress(); }

  void resetStates() { enc.resetStates(); }
};

#endif // ENCODER_H
