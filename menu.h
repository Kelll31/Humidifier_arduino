/*
 * МОДУЛЬ МЕНЮ v2.0
 */

#ifndef MENU_H
#define MENU_H

#include <Arduino.h>
#include "config.h"
#include "display.h"
#include "encoder.h"
#include "storage.h"
#include "sensor.h"
#include "humidifier.h"
#include "analytics.h"

enum MenuItem {
  MENU_MIN_HUMIDITY = 0,
  MENU_MAX_HUMIDITY = 1,
  MENU_HYSTERESIS = 2,
  MENU_CALIBRATE = 3,
  MENU_WATER_THRESHOLD = 4,
  MENU_MANUAL = 5,
  MENU_DISPLAY = 6,
  MENU_RESET_STATS = 7,
  MENU_ABOUT = 8,
  MENU_EXIT = 9,
  MENU_COUNT = 10
};

class Menu {
private:
  Display* display;
  EncoderModule* encoder;
  Storage* storage;
  Sensor* sensor;
  Humidifier* humidifier;
  Analytics* analytics;

  bool active;
  uint8_t currentItem;
  bool editMode;
  int16_t editValue;
  unsigned long lastActivityTime;
  bool menuJustOpened;

  bool calibrationMode;
  uint8_t calibrationStep;
  float tempCalValue;
  float humCalValue;

  bool waterCalMode;
  uint16_t waterThreshold;

  bool manualMode;
  bool manualState;

  bool displaySettingsMode;
  uint8_t displaySubItem;

  bool aboutMode;
  bool needRedraw;

  const char* menuItems[10] = {
    "Минимальная влажность",
    "Макс влажность",
    "Гистерезис",
    "Калибровка",
    "Порог воды",
    "Ручной режим",
    "Настройка дисплея",
    "Сброс статистики",
    "О программе",
    "Выход"
  };

  const char* displayMenuItems[3] = {
    "Яркость",
    "Таймаут",
    "Назад"
  };

public:
  Menu() : display(nullptr), encoder(nullptr), storage(nullptr), 
           sensor(nullptr), humidifier(nullptr), analytics(nullptr),
           active(false), currentItem(0), editMode(false), editValue(0),
           lastActivityTime(0), menuJustOpened(false),
           calibrationMode(false), calibrationStep(0),
           tempCalValue(0), humCalValue(0), waterCalMode(false),
           waterThreshold(WATER_THRESHOLD), manualMode(false), manualState(false),
           displaySettingsMode(false), displaySubItem(0),
           aboutMode(false), needRedraw(true) {}

  void begin(Display* disp, EncoderModule* enc, Storage* stor, Sensor* sens, Humidifier* hum) {
    display = disp;
    encoder = enc;
    storage = stor;
    sensor = sens;
    humidifier = hum;
    analytics = nullptr;
  }

  void setAnalytics(Analytics* ana) { analytics = ana; }

  void open() {
    active = true;
    currentItem = 0;
    menuJustOpened = true;
    resetModes();
    needRedraw = true;
    lastActivityTime = millis();
  }

  void resetModes() {
    editMode = false;
    calibrationMode = false;
    waterCalMode = false;
    manualMode = false;
    displaySettingsMode = false;
    aboutMode = false;
  }

  void close() {
    active = false;
    resetModes();
    storage->save();
  }

  bool isActive() const { return active; }

  void tick() {
    if (!active) return;
    if (millis() - lastActivityTime > SCREEN_TIMEOUT) { close(); return; }

    if (menuJustOpened) {
      menuJustOpened = false;
      return;
    }

    if (encoder->isDouble()) {
      close();
      return;
    }

    if (encoder->isRight()) {
      if (editMode) {
        if (encoder->isFastRotate()) editValue += 5;
        else editValue++;
        switch (currentItem) {
          case MENU_MIN_HUMIDITY: editValue = constrain(editValue, 20, 80); break;
          case MENU_MAX_HUMIDITY: editValue = constrain(editValue, 30, 90); break;
          case MENU_HYSTERESIS: editValue = constrain(editValue, 1, 20); break;
        }
        needRedraw = true;
      }
      else if (calibrationMode) {
        float step = encoder->isFastRotate() ? 0.5 : 0.1;
        if (calibrationStep == 0) {
          tempCalValue += step;
          tempCalValue = constrain(tempCalValue, -10.0, 10.0);
        } else {
          humCalValue += step;
          humCalValue = constrain(humCalValue, -20.0, 20.0);
        }
        needRedraw = true;
      }
      else if (waterCalMode) {
        uint16_t step = encoder->isFastRotate() ? 50 : 10;
        waterThreshold += step;
        waterThreshold = constrain(waterThreshold, 30, 900);
        needRedraw = true;
      }
      else if (manualMode) {
        manualState = !manualState;
        if (manualState) humidifier->turnOn();
        else humidifier->turnOff();
        needRedraw = true;
      }
      else if (displaySettingsMode) {
        displaySubItem++;
        if (displaySubItem >= 3) displaySubItem = 0;
        needRedraw = true;
      }
      else {
        currentItem++;
        if (currentItem >= MENU_COUNT) currentItem = 0;
        needRedraw = true;
      }
      lastActivityTime = millis();
    }

    if (encoder->isLeft()) {
      if (editMode) {
        if (encoder->isFastRotate()) editValue -= 5;
        else editValue--;
        switch (currentItem) {
          case MENU_MIN_HUMIDITY: editValue = constrain(editValue, 20, 80); break;
          case MENU_MAX_HUMIDITY: editValue = constrain(editValue, 30, 90); break;
          case MENU_HYSTERESIS: editValue = constrain(editValue, 1, 20); break;
        }
        needRedraw = true;
      }
      else if (calibrationMode) {
        float step = encoder->isFastRotate() ? 0.5 : 0.1;
        if (calibrationStep == 0) {
          tempCalValue -= step;
          tempCalValue = constrain(tempCalValue, -10.0, 10.0);
        } else {
          humCalValue -= step;
          humCalValue = constrain(humCalValue, -20.0, 20.0);
        }
        needRedraw = true;
      }
      else if (waterCalMode) {
        uint16_t step = encoder->isFastRotate() ? 50 : 10;
        if (waterThreshold > step) waterThreshold -= step;
        else waterThreshold = 30;
        needRedraw = true;
      }
      else if (manualMode) {
        manualState = !manualState;
        if (manualState) humidifier->turnOn();
        else humidifier->turnOff();
        needRedraw = true;
      }
      else if (displaySettingsMode) {
        if (displaySubItem == 0) displaySubItem = 2;
        else displaySubItem--;
        needRedraw = true;
      }
      else {
        if (currentItem == 0) currentItem = MENU_COUNT - 1;
        else currentItem--;
        needRedraw = true;
      }
      lastActivityTime = millis();
    }

    if (encoder->isClick()) {
      if (editMode) {
        switch (currentItem) {
          case MENU_MIN_HUMIDITY: storage->setMinHumidity(editValue); break;
          case MENU_MAX_HUMIDITY: storage->setMaxHumidity(editValue); break;
          case MENU_HYSTERESIS: storage->setHysteresis(editValue); break;
        }
        editMode = false;
      }
      else if (calibrationMode) {
        calibrationStep = (calibrationStep == 0) ? 1 : 0;
      }
      else if (waterCalMode) {
        storage->setWaterThreshold(waterThreshold);
        storage->save();
        waterCalMode = false;
      }
      else if (manualMode) {
        humidifier->exitManualMode();
        manualMode = false;
      }
      else if (displaySettingsMode) {
        if (displaySubItem == 2) {
          displaySettingsMode = false;
        } else if (displaySubItem == 0) {
          uint8_t b = display->getBrightness();
          if (b == BRIGHTNESS_FULL) display->setBrightness(BRIGHTNESS_DIM1);
          else display->setBrightness(BRIGHTNESS_FULL);
        }
      }
      else if (aboutMode) {
        aboutMode = false;
      }
      else {
        selectMenuItem();
      }
      needRedraw = true;
      lastActivityTime = millis();
    }

    if (encoder->isLongPress()) {
      encoder->clearLongPress();
      if (calibrationMode) {
        // При длинном нажатии в режиме калибровки - сохраняем и выходим
        storage->setTempCalibration(tempCalValue);
        storage->setHumCalibration(humCalValue);
        storage->save();
        calibrationMode = false;
      } else if (waterCalMode) {
        // При длинном нажатии устанавливаем порог = текущий уровень воды
        if (analytics) {
          waterThreshold = analytics->getWaterRawValue();
          waterThreshold = constrain(waterThreshold, 30, 900);
          storage->setWaterThreshold(waterThreshold);
          storage->save();
        }
        waterCalMode = false;
      } else {
        close();
      }
      needRedraw = true;
      lastActivityTime = millis();
    }
  }

  void selectMenuItem() {
    switch (currentItem) {
      case MENU_MIN_HUMIDITY: editValue = storage->getMinHumidity(); editMode = true; break;
      case MENU_MAX_HUMIDITY: editValue = storage->getMaxHumidity(); editMode = true; break;
      case MENU_HYSTERESIS: editValue = storage->getHysteresis(); editMode = true; break;
      case MENU_CALIBRATE: calibrationMode = true; calibrationStep = 0; tempCalValue = storage->getTempCalibration(); humCalValue = storage->getHumCalibration(); break;
      case MENU_WATER_THRESHOLD: waterCalMode = true; waterThreshold = storage->getWaterThreshold(); break;
      case MENU_MANUAL: manualMode = true; manualState = humidifier->isRunning(); humidifier->toggle(); break;
      case MENU_DISPLAY: displaySettingsMode = true; displaySubItem = 0; break;
      case MENU_RESET_STATS: storage->resetWorkTime(); storage->resetSwitchCount(); storage->save(); break;
      case MENU_ABOUT: aboutMode = true; break;
      case MENU_EXIT: close(); break;
    }
  }

  void draw() {
    if (!active || !needRedraw) return;
    needRedraw = false;

    if (calibrationMode) {
      display->drawCalibrationScreen(sensor->getTemperature(), sensor->getHumidity(), tempCalValue, humCalValue, (calibrationStep == 0));
      return;
    }
    
    if (waterCalMode) {
      int currentWater = 0;
      uint8_t waterPercent = 0;
      bool waterPresent = false;
      
      if (analytics) {
        currentWater = analytics->getWaterRawValue();
        waterPercent = analytics->getWaterPercent();
        waterPresent = analytics->isWaterSensorPresent();
      }
      
      display->drawWaterCalibrationScreen(currentWater, waterThreshold, waterPresent, waterPercent);
      return;
    }
    
    if (manualMode) {
      display->drawManualScreen(manualState);
      return;
    }
    
    if (displaySettingsMode) {
      drawDisplaySettingsScreen();
      return;
    }
    
    if (aboutMode) {
      display->drawAboutScreen(storage->getWorkTime(), humidifier->getSwitchCount(), storage->getTotalSwitches(), true, waterThreshold, waterThreshold - 50);
      return;
    }
    
    if (editMode) { drawEditScreen(); return; }
    
    drawMenuScreen();
  }

  void drawMenuScreen() {
    display->clear();
    display->setScale(1);
    display->setCursor(40, 0);
    display->print("MENU");
    display->drawLine(0, 10, 127, 10);

    int8_t startItem = currentItem - 2;
    if (startItem < 0) startItem = 0;
    if (startItem > MENU_COUNT - 5) startItem = MENU_COUNT - 5;
    if (startItem < 0) startItem = 0;

    for (uint8_t i = 0; i < 5; i++) {
      uint8_t itemIndex = startItem + i;
      if (itemIndex >= MENU_COUNT) break;
      uint8_t y = 2 + i;
      if (itemIndex == currentItem) { display->setCursor(0, y); display->print(">"); }
      display->setCursor(10, y);
      display->print(menuItems[itemIndex]);
    }

    display->setCursor(0, 7);
    display->print("R-вперед L-наз. DC-exit");
    display->update();
  }

  void drawEditScreen() {
    display->clear();
    display->setScale(1);
    display->setCursor(20, 0);
    display->print("НАСТРОЙКА");
    display->drawLine(0, 10, 127, 10);
    display->setCursor(0, 2);
    display->print(menuItems[currentItem]);
    display->setScale(3);
    display->setCursor(35, 3);
    display->print(editValue);
    display->setScale(1);
    if (currentItem <= MENU_HYSTERESIS) { display->setCursor(95, 5); display->print("%"); }
    display->setCursor(0, 7);
    display->print("R+/- L-наз. CLICK-ок");
    display->update();
  }

  void drawDisplaySettingsScreen() {
    display->clear();
    display->setScale(1);
    display->setCursor(25, 0);
    display->print("ДИСПЛЕЙ");
    display->drawLine(0, 10, 127, 10);

    for (uint8_t i = 0; i < 3; i++) {
      uint8_t y = 2 + i;
      if (i == displaySubItem) { display->setCursor(0, y); display->print(">"); }
      display->setCursor(10, y);
      display->print(displayMenuItems[i]);
    }

    if (displaySubItem == 0) {
      display->setCursor(90, 2);
      uint8_t b = display->getBrightness();
      if (b == BRIGHTNESS_FULL) display->print("100%");
      else if (b == BRIGHTNESS_DIM1) display->print("75%");
      else display->print("20%");
    }

    display->setCursor(0, 7);
    display->print("L-назад");
    display->update();
  }
};

#endif // MENU_H
