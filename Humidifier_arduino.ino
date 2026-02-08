/*
 * АВТОМАТИЧЕСКИЙ УВЛАЖНИТЕЛЬ ВОЗДУХА
 * 
 * Аппаратная платформа: Arduino Nano CH340 (ATmega328P)
 * 
 * Компоненты:
 * - DHT22 (AM2302) на D6 - датчик температуры и влажности
 * - OLED 128x64 I2C (SSD1306) на A4/A5 - дисплей
 * - Энкодер EC11 на D2/D3/D4 - управление меню
 * - MOSFET IRLZ44N на D7 - управление увлажнителем 5V
 * - LED2 на D7 - индикация работы увлажнителя
 * 
 * Автор: Pentester
 * Дата: 2026-02-08
 * Версия: 1.0
 */

#include "config.h"
#include "sensor.h"
#include "display.h"
#include "encoder.h"
#include "humidifier.h"
#include "menu.h"
#include "storage.h"

// Объекты компонентов
Sensor sensor;
Display display;
EncoderModule encoder;
Humidifier humidifier;
Menu menu;
Storage storage;

// Переменные состояния
unsigned long lastUpdateTime = 0;
unsigned long lastSaveTime = 0;

void setup() {
  // Инициализация Serial для отладки
  Serial.begin(9600);
  Serial.println(F("================================="));
  Serial.println(F("  АВТОМАТИЧЕСКИЙ УВЛАЖНИТЕЛЬ"));
  Serial.println(F("================================="));

  // Загрузка настроек из EEPROM
  storage.begin();
  Serial.println(F("[OK] Настройки загружены"));

  // Инициализация дисплея
  display.begin();
  display.showSplash();
  delay(2000);
  Serial.println(F("[OK] Дисплей инициализирован"));

  // Инициализация датчика
  sensor.begin();
  Serial.println(F("[OK] Датчик DHT22 инициализирован"));

  // Инициализация энкодера
  encoder.begin();
  Serial.println(F("[OK] Энкодер инициализирован"));

  // Инициализация увлажнителя
  humidifier.begin();
  Serial.println(F("[OK] Увлажнитель инициализирован"));

  // Инициализация меню
  menu.begin(&display, &encoder, &storage);
  Serial.println(F("[OK] Меню инициализировано"));

  Serial.println(F("================================="));
  Serial.println(F("  СИСТЕМА ГОТОВА К РАБОТЕ!"));
  Serial.println(F("================================="));

  // Включение watchdog timer (защита от зависания)
  wdt_enable(WDTO_4S);
}

void loop() {
  // Сброс watchdog timer
  wdt_reset();

  // Обработка энкодера
  encoder.tick();

  // Обновление данных с датчика каждые 2 секунды
  if (millis() - lastUpdateTime >= UPDATE_INTERVAL) {
    lastUpdateTime = millis();

    // Чтение данных с DHT22
    if (sensor.update()) {
      float temp = sensor.getTemperature();
      float hum = sensor.getHumidity();

      // Логирование данных
      Serial.print(F("Температура: "));
      Serial.print(temp, 1);
      Serial.print(F("°C | Влажность: "));
      Serial.print(hum, 1);
      Serial.println(F("%"));

      // Управление увлажнителем
      humidifier.control(hum, storage.getMinHumidity(), storage.getMaxHumidity());

      // Обновление времени работы
      if (humidifier.isRunning()) {
        storage.incrementWorkTime(UPDATE_INTERVAL / 1000);
      }

    } else {
      Serial.println(F("[ERROR] Ошибка чтения DHT22!"));
      // Мигание LED2 при ошибке
      humidifier.blinkError();
    }
  }

  // Обработка меню и обновление дисплея
  if (menu.isActive()) {
    // Режим меню
    menu.tick();
    menu.draw();
  } else {
    // Главный экран
    display.drawMainScreen(
      sensor.getTemperature(),
      sensor.getHumidity(),
      storage.getMaxHumidity(),
      humidifier.isRunning(),
      storage.getWorkTime()
    );
  }

  // Автосохранение настроек каждые 5 минут
  if (millis() - lastSaveTime >= AUTOSAVE_INTERVAL) {
    lastSaveTime = millis();
    storage.save();
    Serial.println(F("[OK] Автосохранение настроек"));
  }

  // Небольшая задержка для стабильности
  delay(10);
}
