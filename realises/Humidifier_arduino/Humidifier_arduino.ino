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
 * Автор: kelll31
 * Дата: 2026-02-11
 * Версия: 1.7 - Auto Brightness + Humidity Graph
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
unsigned long lastEncoderActivity = 0;
bool dataUpdated = false;

void setup() {
  // Загрузка настроек из EEPROM
  storage.begin();

  // Инициализация дисплея
  display.begin();
  display.showSplash();
  delay(2000);

  // Инициализация датчика
  sensor.begin();
  sensor.setStorage(&storage);

  // Инициализация энкодера
  encoder.begin();

  // Инициализация увлажнителя
  humidifier.begin();
  humidifier.setStorage(&storage);

  // Инициализация меню
  menu.begin(&display, &encoder, &storage, &sensor, &humidifier);

  // Включение watchdog timer
  wdt_enable(WDTO_4S);
  
  // Инициализация таймера активности энкодера
  lastEncoderActivity = millis();
}

void loop() {
  // Сброс watchdog timer
  wdt_reset();

  // Обработка энкодера
  encoder.tick();
  
  // Отслеживание активности энкодера для управления яркостью
  // КРИТИЧНО: используем только isTurn() и isPress() которые НЕ потребляют события
  // НЕ используем isClick(), isRight(), isLeft() - они потребляют события!
  if (encoder.isTurn() || encoder.isPress()) {
    lastEncoderActivity = millis();
    // При любой активности восстанавливаем полную яркость
    if (display.getBrightness() != BRIGHTNESS_FULL) {
      display.setBrightness(BRIGHTNESS_FULL);
    }
  }

  // Автоматическое затемнение дисплея
  unsigned long inactiveTime = millis() - lastEncoderActivity;
  
  if (inactiveTime >= DIM_TIMEOUT_2) {
    // Более 3 минут без активности - 20% яркости
    if (display.getBrightness() != BRIGHTNESS_DIM2) {
      display.setBrightness(BRIGHTNESS_DIM2);
    }
  } else if (inactiveTime >= DIM_TIMEOUT_1) {
    // Более 10 секунд без активности - 75% яркости
    if (display.getBrightness() != BRIGHTNESS_DIM1) {
      display.setBrightness(BRIGHTNESS_DIM1);
    }
  }

  // Обработка storage
  storage.tick();

  // Обновление данных с датчика каждые 2 секунды
  if (millis() - lastUpdateTime >= UPDATE_INTERVAL) {
    lastUpdateTime = millis();

    // Чтение данных с DHT22
    if (sensor.update()) {
      float hum = sensor.getHumidity();
      bool running = humidifier.isRunning();

      // Добавляем точку на график
      display.addGraphPoint(hum, running);

      // Управление увлажнителем
      humidifier.control(hum, storage.getMinHumidity(), storage.getMaxHumidity(), sensor.isOK());

      // Обновление времени работы
      if (humidifier.isRunning()) {
        storage.incrementWorkTime(UPDATE_INTERVAL / 1000);
      }

      dataUpdated = true;

    } else {
      // Мигание LED2 при ошибке
      humidifier.blinkError();
      dataUpdated = true;
    }
  }

  // Обработка меню и обновление дисплея
  if (menu.isActive()) {
    // Режим меню
    menu.tick();
    menu.draw();
  } else {
    // Главный экран
    if (dataUpdated) {
      display.drawMainScreen(
        sensor.getTemperature(),
        sensor.getHumidity(),
        storage.getMaxHumidity(),
        humidifier.isRunning(),
        storage.getWorkTime(),
        sensor.isOK()
      );
      dataUpdated = false;
    }
    
    // Длинное нажатие на главном экране - вход в меню
    if (encoder.isLongPress()) {
      encoder.clearLongPress();
      menu.open();
    }
  }

  // Автосохранение настроек каждые 5 минут
  if (millis() - lastSaveTime >= AUTOSAVE_INTERVAL) {
    lastSaveTime = millis();
    storage.saveDirect();
  }

  // Небольшая задержка для стабильности
  delay(10);
}
