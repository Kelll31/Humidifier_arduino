/*
 * АВТОМАТИЧЕСКИЙ УВЛАЖНИТЕЛЬ v1.7.2
 * 
 * v1.7.2:
 * - 2 режима главного экрана: ДАННЫЕ и ГРАФИК
 * - Переключение вращением энкодера
 * 
 * v1.7:
 * - Датчик уровня воды (A0)
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
  storage.begin();
  
  display.begin();
  display.showSplash();
  delay(2000);
  
  analytics.begin();
  
  if (!analytics.isWaterSensorPresent()) {
    display.showWaterSensorError();
  }
  
  sensor.begin();
  sensor.setStorage(&storage);
  encoder.begin();
  humidifier.begin();
  humidifier.setStorage(&storage);
  menu.begin(&display, &encoder, &storage, &sensor, &humidifier);
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

      bool waterOK = analytics.checkWaterLevel();
      analytics.updateWindowDetector(temp);
      bool windowOpen = analytics.isWindowOpen();

      display.addGraphPoint(hum, running);
      analytics.addSample(temp, hum, running);

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
  
  if (millis() - lastLearningUpdate >= 3600000UL) {
    lastLearningUpdate = millis();
    analytics.updateLearning();
  }

  if (menu.isActive()) {
    menu.tick();
    menu.draw();
  } else {
    // На главном экране - вращение переключает режим
    if (encoder.isRight() || encoder.isLeft()) {
      display.toggleMode();
      dataUpdated = true;
    }
    
    if (dataUpdated) {
      display.drawMainScreen(
        sensor.getTemperature(),
        sensor.getHumidity(),
        storage.getMaxHumidity(),
        humidifier.isRunning(),
        storage.getWorkTime(),
        sensor.isOK(),
        analytics.isWaterLow(),
        analytics.isWindowOpen()
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
    analytics.saveHourlyStats();
    storage.saveDirect();
  }

  delay(10);
}
