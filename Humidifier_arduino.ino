/*
 * АВТОМАТИЧЕСКИЙ УВЛАЖНИТЕЛЬ v1.7.3
 * 
 * v1.7.3:
 * - Добавлена возможность отключения датчика воды
 * - Исправлена проблема с перезагрузкой при старте
 * 
 * v1.7.2:
 * - 2 режима главного экрана
 * 
 * v1.7:
 * - Датчик уровня воды
 * - Детектор открытого окна
 * - Расширенная статистика
 * - Адаптивное обучение
 * 
 * Автор: kelll31 | 2026-02-12
 */

#include "config.h"
#include "sensor.h"
#include "display.h"
#include "encoder.h"
#include "humidifier.h"
#include "menu.h"
#include "storage.h"
#include "analytics.h"

Sensor sensor;
Display display;
EncoderModule encoder;
Humidifier humidifier;
Menu menu;
Storage storage;
Analytics analytics;

unsigned long lastUpdateTime = 0;
unsigned long lastSaveTime = 0;
unsigned long lastEncoderActivity = 0;
unsigned long lastLearningUpdate = 0;
bool dataUpdated = false;

void setup() {
  // Отключаем watchdog на время инициализации
  wdt_disable();
  
  storage.begin();
  
  display.begin();
  display.showSplash();
  delay(1500);
  
  analytics.begin();
  
  sensor.begin();
  sensor.setStorage(&storage);
  encoder.begin();
  humidifier.begin();
  humidifier.setStorage(&storage);
  menu.begin(&display, &encoder, &storage, &sensor, &humidifier);
  
  // Включаем watchdog после инициализации
  wdt_enable(WDTO_4S);
  
  lastEncoderActivity = millis();
  lastLearningUpdate = millis();
}

void loop() {
  wdt_reset();
  encoder.tick();
  
  if (encoder.isTurn() || encoder.isPress()) {
    lastEncoderActivity = millis();
    if (display.getBrightness() != BRIGHTNESS_FULL) {
      display.setBrightness(BRIGHTNESS_FULL);
    }
  }

  unsigned long inactiveTime = millis() - lastEncoderActivity;
  if (inactiveTime >= DIM_TIMEOUT_2) {
    if (display.getBrightness() != BRIGHTNESS_DIM2) display.setBrightness(BRIGHTNESS_DIM2);
  } else if (inactiveTime >= DIM_TIMEOUT_1) {
    if (display.getBrightness() != BRIGHTNESS_DIM1) display.setBrightness(BRIGHTNESS_DIM1);
  }

  storage.tick();

  if (millis() - lastUpdateTime >= UPDATE_INTERVAL) {
    lastUpdateTime = millis();

    if (sensor.update()) {
      float temp = sensor.getTemperature();
      float hum = sensor.getHumidity();
      bool running = humidifier.isRunning();

      #if WATER_SENSOR_ENABLED
        bool waterOK = analytics.checkWaterLevel();
      #else
        bool waterOK = true;
      #endif
      
      #if WINDOW_DETECTOR_ENABLED
        analytics.updateWindowDetector(temp);
        bool windowOpen = analytics.isWindowOpen();
      #else
        bool windowOpen = false;
      #endif

      display.addGraphPoint(hum, running);
      
      #if STATS_ENABLED
        analytics.addSample(temp, hum, running);
      #endif

      if (waterOK && !windowOpen) {
        humidifier.control(hum, storage.getMinHumidity(), storage.getMaxHumidity(), sensor.isOK());
      } else {
        humidifier.stop();
      }

      if (humidifier.isRunning()) {
        storage.incrementWorkTime(UPDATE_INTERVAL / 1000);
      }

      dataUpdated = true;
    } else {
      humidifier.blinkError();
      dataUpdated = true;
    }
  }
  
  #if LEARNING_ENABLED && STATS_ENABLED
    if (millis() - lastLearningUpdate >= 3600000UL) {
      lastLearningUpdate = millis();
      analytics.updateLearning();
    }
  #endif

  if (menu.isActive()) {
    menu.tick();
    menu.draw();
  } else {
    if (encoder.isRight() || encoder.isLeft()) {
      display.toggleMode();
      dataUpdated = true;
    }
    
    if (dataUpdated) {
      #if WATER_SENSOR_ENABLED
        bool waterLow = analytics.isWaterLow();
      #else
        bool waterLow = false;
      #endif
      
      #if WINDOW_DETECTOR_ENABLED
        bool windowOpen = analytics.isWindowOpen();
      #else
        bool windowOpen = false;
      #endif
      
      display.drawMainScreen(
        sensor.getTemperature(),
        sensor.getHumidity(),
        storage.getMaxHumidity(),
        humidifier.isRunning(),
        storage.getWorkTime(),
        sensor.isOK(),
        waterLow,
        windowOpen
      );
      dataUpdated = false;
    }
    
    if (encoder.isLongPress()) {
      encoder.clearLongPress();
      menu.open();
    }
  }

  if (millis() - lastSaveTime >= AUTOSAVE_INTERVAL) {
    lastSaveTime = millis();
    #if STATS_ENABLED
      analytics.saveHourlyStats();
    #endif
    storage.saveDirect();
  }

  delay(10);
}
