/*
<<<<<<< Updated upstream
 * УВЛАЖНИТЕЛЬ v2.2
=======
 * УВЛАЖНИТЕЛЬ v1.9
>>>>>>> Stashed changes
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
unsigned long lastDisplayUpdate = 0;
bool displayNeedsUpdate = false;

void setup() {
  // Сначала Serial для отладки
  Serial.begin(115200);
  Serial.println("=== Humidifier v1.9 ===");
  
  wdt_disable();
  Serial.println("1. Watchdog disabled");
  
  // Сначала инициализируем датчик DHT22
  sensor.begin();
  Serial.println("2. Sensor begin");
  
  // Затем загружаем настройки
  storage.begin();
  Serial.println("3. Storage begin");
  
  // Передаем storage датчику после загрузки
  sensor.setStorage(&storage);
  Serial.println("4. Storage linked to sensor");
  
  // Дисплей
  display.begin();
  Serial.println("5. Display begin");
  
  display.showSplash();
  Serial.println("6. Splash shown");
  delay(1500);
  Serial.println("7. Delay done");
  
  // Аналитика
  analytics.begin();
  Serial.println("8. Analytics begin");
  
  // Энкодер
  encoder.begin();
  Serial.println("9. Encoder begin");
  
  // Увлажнитель
  humidifier.begin();
  humidifier.setStorage(&storage);
  Serial.println("10. Humidifier begin");
  
  // Меню
  menu.begin(&display, &encoder, &storage, &sensor, &humidifier);
  menu.setAnalytics(&analytics);
  Serial.println("11. Menu begin");
  
  wdt_enable(WDTO_4S);
  Serial.println("12. Watchdog enabled");
  
  lastEncoderActivity = millis();
  lastDisplayUpdate = millis();
  Serial.println("=== SETUP COMPLETE ===");
}

void loop() {
  wdt_reset();
  encoder.tick();
  
  // Яркость
  if (encoder.isPress()) {
    lastEncoderActivity = millis();
    display.setBrightness(BRIGHTNESS_FULL);
  }
  unsigned long inactiveTime = millis() - lastEncoderActivity;
  if (inactiveTime >= DIM_TIMEOUT_2) display.setBrightness(BRIGHTNESS_DIM2);
  else if (inactiveTime >= DIM_TIMEOUT_1) display.setBrightness(BRIGHTNESS_DIM1);

  storage.tick();

  // Обновление данных
  if (millis() - lastUpdateTime >= UPDATE_INTERVAL) {
    lastUpdateTime = millis();
    
    Serial.print("Update - DHT22: ");
    bool sensorOK = sensor.update();
    Serial.println(sensorOK ? "OK" : "FAIL");
    
    float temp = sensor.getTemperature();
    float hum = sensor.getHumidity();
    Serial.print("Temp: "); Serial.print(temp);
    Serial.print(" Hum: "); Serial.println(hum);
    
    bool running = humidifier.isRunning();

    bool waterOK = true;
    bool windowOpen = false;
    
    #if WATER_SENSOR_ENABLED
      waterOK = analytics.checkWaterLevel();
    #endif
    
    #if WINDOW_DETECTOR_ENABLED
      analytics.updateWindowDetector(temp);
      windowOpen = analytics.isWindowOpen();
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
    
    // Данные обновились - нужно перерисовать экран
    displayNeedsUpdate = true;
  }

  // Двойной клик - вход/выход из меню
  if (encoder.isDouble()) {
    Serial.println("DOUBLE CLICK");
    if (menu.isActive()) {
      menu.close();
    } else {
      menu.open();
    }
  }

  // Меню
  if (menu.isActive()) {
    menu.tick();
    menu.draw();
  } else {
    // Главный экран - переключение экранов только при вращении вправо
    if (encoder.isRight()) {
      Serial.println("RIGHT");
      if (display.getMode() == MODE_GRAPH) {
        display.toggleGraphScreen();
      } else {
        display.toggleMode();
      }
      displayNeedsUpdate = true;
    }
    
    // Обновляем экран только если нужно
    if (displayNeedsUpdate && millis() - lastDisplayUpdate >= 500) {
      lastDisplayUpdate = millis();
      
      #if WATER_SENSOR_ENABLED
        bool waterLow = analytics.isWaterLow();
        bool waterSensorPresent = analytics.isWaterSensorPresent();
        uint8_t waterPercent = analytics.getWaterPercent();
        int waterRawValue = analytics.getWaterRawValue();
      #else
        bool waterLow = false;
        bool waterSensorPresent = false;
        uint8_t waterPercent = 0;
        int waterRawValue = 0;
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
        windowOpen,
        waterSensorPresent,
        waterPercent,
        waterRawValue
      );
      displayNeedsUpdate = false;
      Serial.println("Screen updated");
    }
  }

  if (millis() - lastSaveTime >= AUTOSAVE_INTERVAL) {
    lastSaveTime = millis();
    storage.saveDirect();
  }

  delay(10);
}
