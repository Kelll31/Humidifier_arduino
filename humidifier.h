/*
 * МОДУЛЬ УПРАВЛЕНИЯ УВЛАЖНИТЕЛЕМ v1.7
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
  Storage* storage;

public:
  Humidifier() : running(false), manualMode(false), lastSwitchTime(0),
                 runStartTime(0), switchCount(0), hourStartTime(0),
                 storage(nullptr) {}

  void setStorage(Storage* stor) {
    storage = stor;
  }

  void begin() {
    pinMode(HUMIDIFIER_PIN, OUTPUT);
    digitalWrite(HUMIDIFIER_PIN, LOW);
    hourStartTime = millis();
  }

  void control(float currentHum, uint8_t minHum, uint8_t maxHum, bool sensorOK) {
    if (manualMode) return;

    if (!sensorOK) {
      if (running) turnOffDirect();
      return;
    }

    unsigned long now = millis();

    if (now - hourStartTime >= 3600000) {
      switchCount = 0;
      hourStartTime = now;
    }

    if (switchCount >= MAX_SWITCHES_PER_HOUR) {
      if (running) turnOff();
      return;
    }

    if (!running && currentHum < minHum) {
      if (now - lastSwitchTime >= MIN_PAUSE_TIME) {
        turnOn();
      }
    }
    else if (running && currentHum >= maxHum) {
      if (now - runStartTime >= MIN_RUN_TIME) {
        turnOff();
      }
    }
  }

  void turnOn() {
    if (!running) {
      digitalWrite(HUMIDIFIER_PIN, HIGH);
      running = true;
      runStartTime = millis();
      lastSwitchTime = millis();
      switchCount++;
      
      if (storage != nullptr) {
        storage->incrementSwitchCount();
      }
    }
  }

  void turnOff() {
    if (running) {
      digitalWrite(HUMIDIFIER_PIN, LOW);
      running = false;
      lastSwitchTime = millis();
    }
  }

  void turnOffDirect() {
    digitalWrite(HUMIDIFIER_PIN, LOW);
    running = false;
  }
  
  // Принудительная остановка (для низкой воды/открытого окна)
  void stop() {
    turnOffDirect();
  }

  void toggle() {
    if (running) {
      turnOff();
    } else {
      turnOn();
    }
    manualMode = true;
  }

  void exitManualMode() {
    manualMode = false;
  }

  bool isRunning() const {
    return running;
  }

  bool isManualMode() const {
    return manualMode;
  }

  unsigned long getRunDuration() const {
    if (running) {
      return (millis() - runStartTime) / 1000;
    }
    return 0;
  }

  void blinkError() {
    static unsigned long lastBlink = 0;
    static bool ledState = false;

    if (millis() - lastBlink >= 250) {
      lastBlink = millis();
      ledState = !ledState;
      digitalWrite(HUMIDIFIER_PIN, ledState);
    }
  }

  uint8_t getSwitchCount() const {
    return switchCount;
  }
};

#endif // HUMIDIFIER_H
