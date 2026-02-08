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
 * Версия: 1.2 - Добавлена полная обработка меню
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
  // Загрузка настроек из EEPROM
  storage.begin();

  // Инициализация дисплея
  display.begin();
  display.showSplash();
  delay(2000);

  // Инициализация датчика
  sensor.begin();

  // Инициализация энкодера
  encoder.begin();

  // Инициализация увлажнителя
  humidifier.begin();

  // Инициализация меню
  menu.begin(&display, &encoder, &storage, &sensor, &humidifier);

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
      float hum = sensor.getHumidity();

      // Управление увлажнителем
      humidifier.control(hum, storage.getMinHumidity(), storage.getMaxHumidity());

      // Обновление времени работы
      if (humidifier.isRunning()) {
        storage.incrementWorkTime(UPDATE_INTERVAL / 1000);
      }

    } else {
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
      storage.getWorkTime(),
      sensor.isOK()
    );
    
    // Длинное нажатие на главном экране - вход в меню
    if (encoder.isLongPress()) {
      encoder.clearLongPress();
      menu.open();
    }
  }

  // Автосохранение настроек каждые 5 минут
  if (millis() - lastSaveTime >= AUTOSAVE_INTERVAL) {
    lastSaveTime = millis();
    storage.save();
  }

  // Небольшая задержка для стабильности
  delay(10);
}
