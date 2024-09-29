#include <string>

#include "esp_log.h"

#include "Display.h"
#include "settings.h"
#include "LEDStrip.h"
#include "TouchController.h"
#include "DisplayManager.h"
#include "misc/SettingsMenu.h"

#define TAG "SettingsMenu"

extern Display* display;
extern LEDStrip* leds;
extern TouchController* touch;

SettingsApp settingsApp = SETTINGS_APP_NONE;
static SettingsMenuState settingsMenuState = SETTINGS_DISPLAY_WIFI;

void SettingsMenu::setup() {};

void SettingsMenu::nextState() {
  settingsMenuState = static_cast<SettingsMenuState>((settingsMenuState + 1) % SETTINGS_DISPLAY_COUNT);
}

void SettingsMenu::prevState() {
  if (settingsMenuState == SETTINGS_DISPLAY_WIFI) {
    settingsMenuState = static_cast<SettingsMenuState>(SETTINGS_DISPLAY_COUNT - 1);
  } else {
    settingsMenuState = static_cast<SettingsMenuState>(settingsMenuState - 1);
  }
}

void SettingsMenu::ontouch(int touchPadNum, const char* eventType, uint32_t touchValue) {
  switch (touchPadNum) {
    case LEFT: {
      SettingsMenu::prevState();
      break;
    }
    case RIGHT: {
      SettingsMenu::nextState();
      break;
    }
    case MIDDLE: {
      if (strcmp(eventType, "longPress") == 0) {
        if (settingsApp == SETTINGS_APP_NONE) {
          DisplayManager::setMenu(MAIN_MENU);
        } else {
          settingsApp = SETTINGS_APP_NONE;
          DisplayManager::appRunning = false;
        }
        return;
      }

      switch (settingsMenuState) {
        case SETTINGS_DISPLAY_WIFI: {
          settingsApp = SETTINGS_APP_WIFI;
          break;
        }
        case SETTINGS_DISPLAY_TOUCH: {
          settingsApp = SETTINGS_APP_TOUCH;
          break;
        }
        default:
          break;
      }

      DisplayManager::appRunning = true;
      leds->stop_animations();

      break;
    }
    default:
      break;
  }
};

void SettingsMenu::render() {
  switch (settingsApp) {
    case SETTINGS_APP_NONE: {
      display->text("settings menu", 0);
      display->text("", 1);

      switch (settingsMenuState) {
        case SETTINGS_DISPLAY_WIFI: {
          display->centeredText("WIFI INFO", 2);
          break;
        }
        case SETTINGS_DISPLAY_TOUCH: {
          display->centeredText("TOUCH SENSOR", 2);
          break;
        }
        default:
          break;
      }

      display->text("", 3);
      break;
    }
    case SETTINGS_APP_WIFI: {
      display->text("WIFI");
      display->text("", 1);
      display->text(WIFI_SSID, 2);
      display->text(WIFI_PASS, 3);
      break;
    }
    case SETTINGS_APP_TOUCH: {
      char line[16];
      sprintf(line, "Touch: %" PRIu32, TOUCH_THRESHOLD);
      display->text(line);
      std::string l = "Left: " + std::to_string(touch->left);
      std::string r = "Right: " + std::to_string(touch->right);
      std::string m = "Middle: " + std::to_string(touch->middle);
      display->text(const_cast<char*>(l.c_str()), 1);
      display->text(const_cast<char*>(r.c_str()), 2);
      display->text(const_cast<char*>(m.c_str()), 3);
      vTaskDelay(25 / portTICK_PERIOD_MS);
      break;
    }
    default:
      break;
  }
};
