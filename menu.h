/*
 * МОДУЛЬ МЕНЮ
 * Навигация и настройка параметров
 * ИСПРАВЛЕНО: правильные координаты для текста
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
enum MenuItem
{
  MENU_MIN_HUMIDITY = 0,
  MENU_MAX_HUMIDITY = 1,
  MENU_HYSTERESIS = 2,
  MENU_CALIBRATE = 3,
  MENU_RESET_STATS = 4,
  MENU_ABOUT = 5,
  MENU_EXIT = 6
};

// Режимы калибровки
enum CalibrationMode
{
  CAL_TEMP = 0,
  CAL_HUM = 1
};

class Menu
{
private:
  Display *display;
  EncoderModule *encoder;
  Storage *storage;
  Sensor *sensor;
  Humidifier *humidifier;

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

  // Флаг необходимости перерисовки экрана меню
  bool needRedraw;

  const char *menuItems[7] = {
      "Мин.влажность",
      "Макс.влажность",
      "Гистерезис",
      "Калибровка",
      "Сброс статистики",
      "О системе",
      "Выход"};

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
           aboutMode(false),
           needRedraw(true) {}

  // Инициализация
  void begin(Display *disp, EncoderModule *enc, Storage *stor, Sensor *sens, Humidifier *hum)
  {
    display = disp;
    encoder = enc;
    storage = stor;
    sensor = sens;
    humidifier = hum;
  }

  // Открыть меню
  void open()
  {
    active = true;
    currentItem = 0;
    editMode = false;
    calibrationMode = false;
    aboutMode = false;
    needRedraw = true;
    encoder->resetPosition();
    lastActivityTime = millis();
  }

  // Закрыть меню
  void close()
  {
    active = false;
    editMode = false;
    calibrationMode = false;
    aboutMode = false;

    // Сохранить настройки при выходе
    storage->save();
  }

  // Проверка активности меню
  bool isActive() const
  {
    return active;
  }

  // Обработка меню
  void tick()
  {
    if (!active)
      return;

    // Автовыход из меню при бездействии
    if (millis() - lastActivityTime > SCREEN_TIMEOUT)
    {
      close();
      return;
    }

    // Обработка режимов
    if (calibrationMode)
    {
      handleCalibrationMode();
    }
    else if (aboutMode)
    {
      handleAboutMode();
    }
    else if (editMode)
    {
      handleEditMode();
    }
    else
    {
      handleNavigationMode();
    }
  }

  // Режим навигации
  void handleNavigationMode()
  {
    // Обработка вращения энкодера
    int8_t delta = encoder->getDelta();
    if (delta != 0)
    {
      currentItem += delta;

      // Циклическая навигация
      if (currentItem < 0)
        currentItem = MENU_EXIT;
      if (currentItem > MENU_EXIT)
        currentItem = 0;

      lastActivityTime = millis();
      needRedraw = true;
    }

    // Короткое нажатие - выбор пункта
    if (encoder->isClick())
    {
      selectMenuItem();
      lastActivityTime = millis();
      needRedraw = true;
    }

    // Длинное нажатие - выход из меню
    if (encoder->isLongPress())
    {
      encoder->clearLongPress();
      close();
    }
  }

  // Режим редактирования
  void handleEditMode()
  {
    int8_t delta = encoder->getDelta();

    if (delta != 0)
    {
      // Быстрое вращение - изменение по 5
      if (encoder->isFastRotate())
      {
        editValue += delta * 5;
      }
      else
      {
        editValue += delta;
      }

      // Ограничение значений
      switch (currentItem)
      {
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
      needRedraw = true;
    }

    // Короткое нажатие - сохранить
    if (encoder->isClick())
    {
      saveEditValue();
      editMode = false;
      encoder->resetPosition();
      lastActivityTime = millis();
      needRedraw = true;
    }

    // Длинное нажатие - отмена
    if (encoder->isLongPress())
    {
      encoder->clearLongPress();
      editMode = false;
      encoder->resetPosition();
      lastActivityTime = millis();
      needRedraw = true;
    }
  }

  // Режим калибровки
  void handleCalibrationMode()
  {
    int8_t delta = encoder->getDelta();

    if (delta != 0)
    {
      float step = 0.1;
      if (encoder->isFastRotate())
      {
        step = 0.5;
      }

      if (calibrationStep == CAL_TEMP)
      {
        tempCalValue += delta * step;
        tempCalValue = constrain(tempCalValue, -10.0, 10.0);
      }
      else
      {
        humCalValue += delta * step;
        humCalValue = constrain(humCalValue, -20.0, 20.0);
      }

      lastActivityTime = millis();
      needRedraw = true;
    }

    // Короткое нажатие - переключение между Т и В
    if (encoder->isClick())
    {
      calibrationStep = (calibrationStep == CAL_TEMP) ? CAL_HUM : CAL_TEMP;
      encoder->resetPosition();
      lastActivityTime = millis();
      needRedraw = true;
    }

    // Длинное нажатие - сохранить и выйти
    if (encoder->isLongPress())
    {
      encoder->clearLongPress();
      storage->setTempCalibration(tempCalValue);
      storage->setHumCalibration(humCalValue);
      storage->save();
      calibrationMode = false;
      encoder->resetPosition();
      lastActivityTime = millis();
      needRedraw = true;
    }
  }

  // Режим "О системе"
  void handleAboutMode()
  {
    // Длинное нажатие - выход
    if (encoder->isLongPress())
    {
      encoder->clearLongPress();
      aboutMode = false;
      encoder->resetPosition();
      lastActivityTime = millis();
      needRedraw = true;
    }
  }

  // Выбор пункта меню
  void selectMenuItem()
  {
    switch (currentItem)
    {
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
  void saveEditValue()
  {
    switch (currentItem)
    {
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
  void draw()
  {
    if (!active)
      return;

    // Ничего не делаем, если экран не требует перерисовки
    if (!needRedraw)
      return;
    needRedraw = false;

    if (calibrationMode)
    {
      display->drawCalibrationScreen(
          sensor->getTemperature(),
          sensor->getHumidity(),
          tempCalValue,
          humCalValue,
          (calibrationStep == CAL_TEMP));
      return;
    }

    if (aboutMode)
    {
      display->drawAboutScreen(
          storage->getWorkTime(),
          humidifier->getSwitchCount(),
          storage->getTotalSwitches());
      return;
    }

    if (editMode)
    {
      drawEditScreen();
      return;
    }

    drawMenuScreen();
  }

  // Отрисовка экрана меню - ИСПРАВЛЕНО
  void drawMenuScreen()
  {
    display->clear();

    // Заголовок - используем setCursorXY с пиксельными координатами
    display->setScale(1);
    display->setCursor(5, 0); // x=5 символов, y=0 строка
    display->print(F("МЕНЮ"));
    display->drawLine(0, 10, 127, 10);

    // Отображаем 5 пунктов: текущий, 2 предыдущих, 2 следующих
    int8_t startItem = currentItem - 2;
    if (startItem < 0)
      startItem = 0;
    if (startItem > MENU_EXIT - 4)
      startItem = MENU_EXIT - 4;
    if (startItem < 0)
      startItem = 0;

    for (uint8_t i = 0; i < 5; i++)
    {
      uint8_t itemIndex = startItem + i;
      if (itemIndex > MENU_EXIT)
        break;

      uint8_t yRow = 2 + i; // Текстовая строка (строка 2, 3, 4, 5, 6)

      // Стрелка "> " для текущего пункта
      if (itemIndex == currentItem)
      {
        display->setCursor(0, yRow);
        display->print(F(">"));
      }

      // Текст пункта
      display->setCursor(2, yRow); // x=2 символа (отступ от стрелки)
      display->print(menuItems[itemIndex]);
    }

    // Подсказка
    display->setCursor(0, 7); // Строка 7 (последняя)
    display->print(F("ДН-выб ДЛ-выход"));

    display->update();
  }

  // Отрисовка экрана редактирования - ИСПРАВЛЕНО
  void drawEditScreen()
  {
    display->clear();

    display->setScale(1);
    display->setCursor(2, 0); // x=2 символа, y=0 строка
    display->print(F("НАСТРОЙКА"));
    display->drawLine(0, 10, 127, 10);

    // Название параметра
    display->setCursor(0, 2); // Строка 2
    display->print(menuItems[currentItem]);

    // Значение - большой шрифт
    display->setScale(3);
    display->setCursor(4, 3); // Центрируем, строка 3
    display->print(editValue);

    display->setScale(1);

    // Единицы измерения
    if (currentItem <= MENU_HYSTERESIS)
    {
      display->setCursor(12, 5); // Строка 5
      display->print(F("%"));
    }

    // Подсказка
    display->setCursor(0, 7); // Строка 7
    display->print(F("ДН-OK ДЛ-отмена"));

    display->update();
  }
};

#endif // MENU_H
