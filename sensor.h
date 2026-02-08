/*
 * МОДУЛЬ ДАТЧИКА DHT22
 * Чтение температуры и влажности
 */

#ifndef SENSOR_H
#define SENSOR_H

#include <Arduino.h>
#include <DHT.h>
#include "config.h"

class Sensor {
private:
  DHT dht;
  float temperature;
  float humidity;
  bool lastReadSuccess;
  unsigned long lastReadTime;
  uint8_t errorCount;

public:
  Sensor() : dht(DHT_PIN, DHT_TYPE), 
             temperature(0), 
             humidity(0), 
             lastReadSuccess(false),
             lastReadTime(0),
             errorCount(0) {}

  // Инициализация датчика
  void begin() {
    dht.begin();
    delay(2000); // DHT22 требует задержку после инициализации
  }

  // Обновление данных с датчика
  bool update() {
    // Защита от слишком частого опроса
    if (millis() - lastReadTime < 2000) {
      return lastReadSuccess;
    }

    lastReadTime = millis();

    // Чтение данных
    float h = dht.readHumidity();
    float t = dht.readTemperature();

    // Проверка корректности данных
    if (isnan(h) || isnan(t)) {
      errorCount++;
      lastReadSuccess = false;

      DEBUG_PRINT(F("[DHT22] Ошибка чтения. Попытка: "));
      DEBUG_PRINTLN(errorCount);

      // При 5 ошибках подряд - критическая ошибка
      if (errorCount >= 5) {
        DEBUG_PRINTLN(F("[DHT22] КРИТИЧЕСКАЯ ОШИБКА!"));
      }

      return false;
    }

    // Применение калибровки
    temperature = t + TEMP_CALIBRATION;
    humidity = h + HUM_CALIBRATION;

    // Ограничение влажности в диапазоне 0-100%
    humidity = constrain(humidity, 0.0, 100.0);

    errorCount = 0;
    lastReadSuccess = true;

    return true;
  }

  // Получение температуры
  float getTemperature() const {
    return temperature;
  }

  // Получение влажности
  float getHumidity() const {
    return humidity;
  }

  // Проверка состояния датчика
  bool isOK() const {
    return lastReadSuccess && (errorCount < 5);
  }

  // Получение количества ошибок
  uint8_t getErrorCount() const {
    return errorCount;
  }
};

#endif // SENSOR_H
