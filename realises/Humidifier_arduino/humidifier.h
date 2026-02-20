/*
 * МОДУЛЬ УПРАВЛЕНИЯ УВЛАЖНИТЕЛЕМ
 * Управление MOSFET с защитой от частых переключений
 */

#ifndef HUMIDIFIER_H
#define HUMIDIFIER_H

#include <Arduino.h>
#include "config.h"
#include "storage.h"

class Humidifier {
private:
  bool running;
  bool manualMode;
  unsigned long lastSwitchTime;
  unsigned long runStartTime;
  uint8_t switchCount;
  unsigned long hourStartTime;
  
  // Указатель на storage
  Storage* storage;

public:
  Humidifier() : running(false),
                 manualMode(false),
                 lastSwitchTime(0),
                 runStartTime(0),
                 switchCount(0),
                 hourStartTime(0),
                 storage(nullptr) {}

  // Установка ссылки на storage
  void setStorage(Storage* stor) {
    storage = stor;
  }

  // Инициализация
  void begin() {
    pinMode(HUMIDIFIER_PIN, OUTPUT);
    digitalWrite(HUMIDIFIER_PIN, LOW);
    hourStartTime = millis();
  }

  // Автоматическое управление
  void control(float currentHum, uint8_t minHum, uint8_t maxHum, bool sensorOK) {
    // Пропускаем, если ручной режим
    if (manualMode) return;

    // Выключаем при ошибке датчика
    if (!sensorOK) {
      if (running) {
        turnOffDirect(); // Прямое выключение без учета статистики
      }
      return;
    }

    unsigned long now = millis();

    // Сброс счетчика переключений каждый час
    if (now - hourStartTime >= 3600000) {
      switchCount = 0;
      hourStartTime = now;
    }

    // Проверка на максимум переключений
    if (switchCount >= MAX_SWITCHES_PER_HOUR) {
      if (running) {
        turnOff();
      }
      return;
    }

    // Логика управления с гистерезисом
    if (!running && currentHum < minHum) {
      // Включить увлажнитель
      if (now - lastSwitchTime >= MIN_PAUSE_TIME) {
        turnOn();
      }
    }
    else if (running && currentHum >= maxHum) {
      // Выключить увлажнитель
      if (now - runStartTime >= MIN_RUN_TIME) {
        turnOff();
      }
    }
  }

  // Включение
  void turnOn() {
    if (!running) {
      digitalWrite(HUMIDIFIER_PIN, HIGH);
      running = true;
      runStartTime = millis();
      lastSwitchTime = millis();
      switchCount++;
      
      // Учет общего количества переключений
      if (storage != nullptr) {
        storage->incrementSwitchCount();
      }
    }
  }

  // Выключение
  void turnOff() {
    if (running) {
      digitalWrite(HUMIDIFIER_PIN, LOW);
      running = false;
      lastSwitchTime = millis();
    }
  }

  // Прямое выключение (без учета)
  void turnOffDirect() {
    digitalWrite(HUMIDIFIER_PIN, LOW);
    running = false;
  }

  // Переключение (для ручного управления)
  void toggle() {
    if (running) {
      turnOff();
    } else {
      turnOn();
    }
    manualMode = true;
  }

  // Выход из ручного режима
  void exitManualMode() {
    manualMode = false;
  }

  // Проверка состояния
  bool isRunning() const {
    return running;
  }

  // Проверка ручного режима
  bool isManualMode() const {
    return manualMode;
  }

  // Получение времени работы в текущем цикле
  unsigned long getRunDuration() const {
    if (running) {
      return (millis() - runStartTime) / 1000;
    }
    return 0;
  }

  // Мигание при ошибке датчика
  void blinkError() {
    static unsigned long lastBlink = 0;
    static bool ledState = false;

    if (millis() - lastBlink >= 250) {
      lastBlink = millis();
      ledState = !ledState;
      digitalWrite(HUMIDIFIER_PIN, ledState);
    }
  }

  // Получение количества переключений
  uint8_t getSwitchCount() const {
    return switchCount;
  }
};

#endif // HUMIDIFIER_H
