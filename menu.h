/*
 * МОДУЛЬ МЕНЮ
 * Навигация и настройка параметров
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

// Пункты меню
enum MenuItem {
  MENU_MIN_HUMIDITY = 0,
  MENU_MAX_HUMIDITY = 1,
  MENU_HYSTERESIS = 2,
  MENU_CALIBRATE = 3,
  MENU_RESET_STATS = 4,
  MENU_ABOUT = 5,
  MENU_EXIT = 6
};

// Режимы калибровки
enum CalibrationMode {
  CAL_TEMP = 0,
  CAL_HUM = 1
};

class Menu {
private:
  Display* display;
  EncoderModule* encoder;
  Storage* storage;
  Sensor* sensor;
  Humidifier* humidifier;

  bool active;
  uint8_t currentItem;
  bool editMode;
  int16_t editValue;
  unsigned long lastActivityTime;

  // Калибровка
  bool calibrationMode;
  uint8_t calibrationStep;
  float tempCalValue;
  float humCalValue;

  // Экран "О системе"
  bool aboutMode;

  const char* menuItems[7] = {
    "1.Мин.влажность",
    "2.Макс.влажность",
    "3.Гистерезис",
    "4.Калибровка",
    "5.Сброс статистики",
    "6.О системе",
    "7.Выход"
  };

public:
  Menu() : display(nullptr),
           encoder(nullptr),
           storage(nullptr),
           sensor(nullptr),
           humidifier(nullptr),
           active(false),
           currentItem(0),
           editMode(false),
           editValue(0),
           lastActivityTime(0),
           calibrationMode(false),
           calibrationStep(0),
           tempCalValue(0),
           humCalValue(0),
           aboutMode(false) {}

  // Инициализация
  void begin(Display* disp, EncoderModule* enc, Storage* stor, Sensor* sens, Humidifier* hum) {
    display = disp;
    encoder = enc;
    storage = stor;
    sensor = sens;
    humidifier = hum;
  }

  // Открыть меню
  void open() {
    active = true;
    currentItem = 0;
    editMode = false;
    calibrationMode = false;
    aboutMode = false;
    encoder->resetPosition();
    lastActivityTime = millis();
  }

  // Закрыть меню
  void close() {
    active = false;
    editMode = false;
    calibrationMode = false;
    aboutMode = false;

    // Сохранить настройки при выходе
    storage->save();
  }

  // Проверка активности меню
  bool isActive() const {
    return active;
  }

  // Обработка меню
  void tick() {
    if (!active) return;

    // Автовыход из меню при бездействии
    if (millis() - lastActivityTime > SCREEN_TIMEOUT) {
      close();
      return;
    }

    // Обработка режимов
    if (calibrationMode) {
      handleCalibrationMode();
    } else if (aboutMode) {
      handleAboutMode();
    } else if (editMode) {
      handleEditMode();
    } else {
      handleNavigationMode();
    }
  }

  // Режим навигации
  void handleNavigationMode() {
    // Обработка вращения энкодера
    int8_t delta = encoder->getDelta();
    if (delta != 0) {
      currentItem += delta;

      // Циклическая навигация
      if (currentItem < 0) currentItem = MENU_EXIT;
      if (currentItem > MENU_EXIT) currentItem = 0;

      lastActivityTime = millis();
    }

    // Короткое нажатие - выбор пункта
    if (encoder->isClick()) {
      selectMenuItem();
      lastActivityTime = millis();
    }

    // Длинное нажатие - выход из меню
    if (encoder->isLongPress()) {
      encoder->clearLongPress();
      close();
    }
  }

  // Режим редактирования
  void handleEditMode() {
    int8_t delta = encoder->getDelta();

    if (delta != 0) {
      // Быстрое вращение - изменение по 5
      if (encoder->isFastRotate()) {
        editValue += delta * 5;
      } else {
        editValue += delta;
      }

      // Ограничение значений
      switch (currentItem) {
        case MENU_MIN_HUMIDITY:
          editValue = constrain(editValue, 20, 80);
          break;
        case MENU_MAX_HUMIDITY:
          editValue = constrain(editValue, 30, 90);
          break;
        case MENU_HYSTERESIS:
          editValue = constrain(editValue, 1, 20);
          break;
      }

      lastActivityTime = millis();
    }

    // Короткое нажатие - сохранить
    if (encoder->isClick()) {
      saveEditValue();
      editMode = false;
      encoder->resetPosition();
      lastActivityTime = millis();
    }

    // Длинное нажатие - отмена
    if (encoder->isLongPress()) {
      encoder->clearLongPress();
      editMode = false;
      encoder->resetPosition();
      lastActivityTime = millis();
    }
  }

  // Режим калибровки
  void handleCalibrationMode() {
    int8_t delta = encoder->getDelta();

    if (delta != 0) {
      float step = 0.1;
      if (encoder->isFastRotate()) {
        step = 0.5;
      }

      if (calibrationStep == CAL_TEMP) {
        tempCalValue += delta * step;
        tempCalValue = constrain(tempCalValue, -10.0, 10.0);
      } else {
        humCalValue += delta * step;
        humCalValue = constrain(humCalValue, -20.0, 20.0);
      }

      lastActivityTime = millis();
    }

    // Короткое нажатие - переключение между Т и В
    if (encoder->isClick()) {
      calibrationStep = (calibrationStep == CAL_TEMP) ? CAL_HUM : CAL_TEMP;
      encoder->resetPosition();
      lastActivityTime = millis();
    }

    // Длинное нажатие - сохранить и выйти
    if (encoder->isLongPress()) {
      encoder->clearLongPress();
      storage->setTempCalibration(tempCalValue);
      storage->setHumCalibration(humCalValue);
      storage->save();
      calibrationMode = false;
      encoder->resetPosition();
      lastActivityTime = millis();
    }
  }

  // Режим "О системе"
  void handleAboutMode() {
    // Длинное нажатие - выход
    if (encoder->isLongPress()) {
      encoder->clearLongPress();
      aboutMode = false;
      encoder->resetPosition();
      lastActivityTime = millis();
    }
  }

  // Выбор пункта меню
  void selectMenuItem() {
    switch (currentItem) {
      case MENU_MIN_HUMIDITY:
        editValue = storage->getMinHumidity();
        editMode = true;
        encoder->resetPosition();
        break;

      case MENU_MAX_HUMIDITY:
        editValue = storage->getMaxHumidity();
        editMode = true;
        encoder->resetPosition();
        break;

      case MENU_HYSTERESIS:
        editValue = storage->getHysteresis();
        editMode = true;
        encoder->resetPosition();
        break;

      case MENU_CALIBRATE:
        // Вход в режим калибровки
        calibrationMode = true;
        calibrationStep = CAL_TEMP;
        tempCalValue = storage->getTempCalibration();
        humCalValue = storage->getHumCalibration();
        encoder->resetPosition();
        break;

      case MENU_RESET_STATS:
        storage->resetWorkTime();
        storage->resetSwitchCount();
        storage->save();
        break;

      case MENU_ABOUT:
        // Показать экран "О системе"
        aboutMode = true;
        break;

      case MENU_EXIT:
        close();
        break;
    }
  }

  // Сохранение отредактированного значения
  void saveEditValue() {
    switch (currentItem) {
      case MENU_MIN_HUMIDITY:
        storage->setMinHumidity(editValue);
        break;

      case MENU_MAX_HUMIDITY:
        storage->setMaxHumidity(editValue);
        break;

      case MENU_HYSTERESIS:
        storage->setHysteresis(editValue);
        break;
    }
  }

  // Отрисовка меню
  void draw() {
    if (!active) return;

    display->clear();

    if (calibrationMode) {
      display->drawCalibrationScreen(
        sensor->getTemperature(),
        sensor->getHumidity(),
        tempCalValue,
        humCalValue,
        (calibrationStep == CAL_TEMP)
      );
    } else if (aboutMode) {
      display->drawAboutScreen(
        storage->getWorkTime(),
        humidifier->getSwitchCount(),
        storage->getTotalSwitches()
      );
    } else if (editMode) {
      drawEditScreen();
    } else {
      drawMenuScreen();
    }

    display->update();
  }

  // Отрисовка экрана меню
  void drawMenuScreen() {
    // Заголовок
    display->setScale(1);
    display->setCursor(40, 0);
    display->print(F("МЕНЮ"));
    display->drawLine(0, 10, 127, 10);

    // Отображаем 3 пункта: текущий, предыдущий, следующий
    int8_t startItem = currentItem - 1;
    if (startItem < 0) startItem = 0;

    for (uint8_t i = 0; i < 3; i++) {
      uint8_t itemIndex = startItem + i;
      if (itemIndex > MENU_EXIT) break;

      uint8_t y = 2 + i * 2;

      // Выделение текущего пункта
      if (itemIndex == currentItem) {
        display->drawRect(0, y * 8, 127, y * 8 + 10, true);
      }

      display->setCursor(2, y);
      display->print(menuItems[itemIndex]);
    }

    // Подсказка
    display->setCursor(0, 7);
    display->print(F("ДН-выб ДЛ-выход"));
  }

  // Отрисовка экрана редактирования
  void drawEditScreen() {
    display->setScale(1);
    display->setCursor(20, 0);
    display->print(F("НАСТРОЙКА"));
    display->drawLine(0, 10, 127, 10);

    // Название параметра
    display->setCursor(0, 2);
    display->print(menuItems[currentItem] + 2); // Пропускаем "1."

    // Значение
    display->setScale(3);
    display->setCursor(35, 3);
    display->print(editValue);

    display->setScale(1);

    // Единицы измерения
    if (currentItem <= MENU_HYSTERESIS) {
      display->setCursor(95, 5);
      display->print(F("%"));
    }

    // Подсказка
    display->setCursor(0, 7);
    display->print(F("ДН-OK ДЛ-отмена"));
  }
};

#endif // MENU_H
