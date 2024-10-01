#include <string>

#include "settings.h"
#include "Display.h"
#include "LEDStrip.h"
#include "TouchController.h"
#include "DisplayManager.h"
#include "ble/BLEMenu.h"
#include "wifi/WIFIMenu.h"
#include "wifi/WebServerModule.h"
#include "misc/SettingsMenu.h"

#define TAG "DisplayMan"

LEDStrip* leds;
Display* display;
TouchController* touch;
WebServerModule* webServer;

static MenuState currentMenu = MAIN_MENU;
static MainMenuState mainMenuState = DISPLAY_NAME;

static int64_t idleTime = 0;

bool DisplayManager::appRunning = false;

void DisplayManager::setup() {
  read_settings();
  leds = new LEDStrip();
  display = new Display();
  touch = new TouchController(TOUCH_PAD_NUM9, TOUCH_PAD_NUM1, TOUCH_PAD_NUM6);
  webServer = new WebServerModule(WIFI_SSID, WIFI_PASS);

  leds->set_color(0, 255, 0);
  
  // Setup the touch controller with a lambda function as the callback
  touch->setup([](int touchPadNum, const char* eventType, uint32_t touchValue) {
    leds->set_color(0, 0, 0); // clear led colors upon release
    
    if (touch->lock) {
      return;
    }

    idleTime = esp_timer_get_time(); // resume led animations when not used for a while

    if (currentMenu == BLE_MENU) {
      return BLEMenu::ontouch(touchPadNum, eventType, touchValue);
    }

    if (currentMenu == WIFI_MENU) {
      return WIFIMenu::ontouch(touchPadNum, eventType, touchValue);
    }

    if (currentMenu == SETTINGS_MENU) {
      return SettingsMenu::ontouch(touchPadNum, eventType, touchValue);
    }

    switch (touchPadNum) {
      case RIGHT: {
        if (currentMenu == MAIN_MENU) {
          DisplayManager::nextState();
        }
        break;
      }
      case LEFT: {
        if (currentMenu == MAIN_MENU) {
          DisplayManager::prevState();
        }
        break;
      }
      case MIDDLE: {
        if (strcmp(eventType, "longPress") == 0) {
          if (currentMenu != MAIN_MENU) {
            currentMenu = MAIN_MENU;
          }
          return;
        }
        
        if (currentMenu == MAIN_MENU) {
          if (mainMenuState == DISPLAY_WIFI) {
            currentMenu = WIFI_MENU;
          }
          if (mainMenuState == DISPLAY_BLE) {
            currentMenu = BLE_MENU;
          }
          if (mainMenuState == DISPLAY_SETTINGS) {
            currentMenu = SETTINGS_MENU;
          }
        }
        break;
      }
      default:
        break;
    }
  });

  display->centeredText("WABBT", 1);
  display->centeredText(version, 2);
  vTaskDelay(3000 / portTICK_PERIOD_MS);
  leds->run_animations();
  webServer->setup();
};

void DisplayManager::setMenu(MenuState menu) {
  currentMenu = menu;
};

void DisplayManager::nextMenu() {
  currentMenu = static_cast<MenuState>((currentMenu + 1) % MENU_STATE_COUNT);
};

void DisplayManager::prevMenu() {
  if (currentMenu == MAIN_MENU) {
    currentMenu = static_cast<MenuState>(MENU_STATE_COUNT - 1);
  } else {
    currentMenu = static_cast<MenuState>(currentMenu - 1);
  }
};

void DisplayManager::nextState() {
  mainMenuState = static_cast<MainMenuState>((mainMenuState + 1) % DISPLAY_STATE_COUNT);
};

void DisplayManager::prevState() {
  if (mainMenuState == DISPLAY_NAME) {
    mainMenuState = static_cast<MainMenuState>(DISPLAY_STATE_COUNT - 1);
  } else {
    mainMenuState = static_cast<MainMenuState>(mainMenuState - 1);
  }
};

void DisplayManager::render() {
  webServer->loop();

  if (display->connected) {
    switch (currentMenu) {
      case MAIN_MENU: {

        display->text("", 0);
        display->text("", 1);

        switch (mainMenuState) {
          case DISPLAY_NAME: {
            display->centeredText((const char*)badge_name, 2);
            break;
          }
          case DISPLAY_WIFI:
            display->centeredText("WIFI MENU", 2);
            break;
          case DISPLAY_BLE:
            display->centeredText("BLE MENU", 2);
            break;
          case DISPLAY_SETTINGS:
            display->centeredText("SETTINGS", 2);
          default:
            break;
        }

        display->text("", 3);

        break;
      }
      case BLE_MENU: {
        BLEMenu::render();
        break;
      }
      case WIFI_MENU: {
        WIFIMenu::render();
        break;
      }
      case SETTINGS_MENU: {
        SettingsMenu::render();
        break;
      }
      default:
        break;
    }
  }

  if (!appRunning) {
    int64_t idle = esp_timer_get_time() - idleTime;
    if (idle >= IDLETIMER) {
      if (!LEDStrip::animationRunning) {
        idleTime = esp_timer_get_time();
        leds->run_animations();
        vTaskDelay(10 / portTICK_PERIOD_MS);
      }
    }
  }
};
