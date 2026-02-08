/*
 * МОДУЛЬ ДАТЧИКА DHT22
 * Чтение температуры и влажности
 */

#ifndef SENSOR_H
#define SENSOR_H

#include <Arduino.h>
#include <DHT.h>
#include "config.h"
#include "storage.h"

class Sensor {
private:
  DHT dht;
  float temperature;
  float humidity;
  float rawTemperature;
  float rawHumidity;
  bool lastReadSuccess;
  unsigned long lastReadTime;
  uint8_t errorCount;
  uint8_t consecutiveErrors;
  
  // Указатель на storage для калибровки
  Storage* storage;

public:
  Sensor() : dht(DHT_PIN, DHT_TYPE), 
             temperature(0), 
             humidity(0),
             rawTemperature(0),
             rawHumidity(0),
             lastReadSuccess(false),
             lastReadTime(0),
             errorCount(0),
             consecutiveErrors(0),
             storage(nullptr) {}

  // Установка ссылки на storage
  void setStorage(Storage* stor) {
    storage = stor;
  }

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
      handleError();
      return false;
    }

    // Проверка диапазона значений
    if (t < -40.0 || t > 80.0 || h < 0.0 || h > 100.0) {
      handleError();
      return false;
    }

    // Сохранение сырых значений
    rawTemperature = t;
    rawHumidity = h;

    // Применение калибровки из storage
    if (storage != nullptr) {
      temperature = t + storage->getTempCalibration();
      humidity = h + storage->getHumCalibration();
    } else {
      temperature = t;
      humidity = h;
    }

    // Ограничение в допустимых пределах
    temperature = constrain(temperature, -40.0, 80.0);
    humidity = constrain(humidity, 0.0, 100.0);

    // Успешное чтение
    consecutiveErrors = 0;
    lastReadSuccess = true;

    return true;
  }

  // Обработка ошибки
  void handleError() {
    consecutiveErrors++;
    if (errorCount < 255) {
      errorCount++;
    }
    lastReadSuccess = false;
  }

  // Получение температуры
  float getTemperature() const {
    return temperature;
  }

  // Получение влажности
  float getHumidity() const {
    return humidity;
  }

  // Получение сырой температуры (без калибровки)
  float getRawTemperature() const {
    return rawTemperature;
  }

  // Получение сырой влажности (без калибровки)
  float getRawHumidity() const {
    return rawHumidity;
  }

  // Проверка состояния датчика
  bool isOK() const {
    return lastReadSuccess && (consecutiveErrors < 3);
  }

  // Критическая ошибка
  bool isCriticalError() const {
    return consecutiveErrors >= 5;
  }

  // Получение количества ошибок
  uint8_t getErrorCount() const {
    return errorCount;
  }

  // Получение последовательных ошибок
  uint8_t getConsecutiveErrors() const {
    return consecutiveErrors;
  }

  // Сброс счетчика ошибок
  void resetErrorCount() {
    errorCount = 0;
    consecutiveErrors = 0;
  }
};

#endif // SENSOR_H
