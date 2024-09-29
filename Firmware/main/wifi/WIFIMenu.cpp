#include <Arduino.h>
#include "esp_log.h"

#include "wifi/DeauthApp.h"
#include "wifi/DeauthDetectApp.h"
#include "wifi/PMKIDApp.h"
#include "wifi/WIFIMenu.h"
#include "Display.h"
#include "DisplayManager.h"
#include "settings.h"

#define TAG "WIFIMenu"

extern Display* display;

WIFIApp wifiApp = WIFI_APP_NONE;
static WIFIMenuState wifiMenuState = WIFI_DISPLAY_DEAUTH;

void WIFIMenu::setup() {};

void WIFIMenu::nextState() {
  wifiMenuState = static_cast<WIFIMenuState>((wifiMenuState + 1) % WIFI_DISPLAY_COUNT);
}

void WIFIMenu::prevState() {
  if (wifiMenuState == WIFI_DISPLAY_DEAUTH) {
    wifiMenuState = static_cast<WIFIMenuState>(WIFI_DISPLAY_COUNT - 1);
  } else {
    wifiMenuState = static_cast<WIFIMenuState>(wifiMenuState - 1);
  }
}

void WIFIMenu::ontouch(int pad, const char* type, uint32_t value) {

  if (wifiApp == WIFI_APP_DEAUTH) {
    DeauthApp::ontouch(pad, type, value);
    return;
  }

  if (wifiApp == WIFI_APP_DEAUTH_DETECT) {
    DeauthDetectApp::ontouch(pad, type, value);
    return;
  }

  if (wifiApp == WIFI_APP_PMKID) {
    PMKIDApp::ontouch(pad, type, value);
    return;
  }

  switch (pad) {
    case LEFT: {
      WIFIMenu::prevState();
      break;
    }
    case RIGHT: {
      WIFIMenu::nextState();
      break;
    }
    case MIDDLE: {
      if (strcmp(type, "longPress") == 0) {
        if (wifiApp == WIFI_APP_NONE) {
          DisplayManager::setMenu(MAIN_MENU);          
        } else {
          wifiApp = WIFI_APP_NONE;
          DisplayManager::appRunning = false;
        }
        return;
      }

      switch (wifiMenuState) {
        case WIFI_DISPLAY_DEAUTH: {
          wifiApp = WIFI_APP_DEAUTH;
          break;
        }
        case WIFI_DISPLAY_DEAUTH_DETECT: {
          wifiApp = WIFI_APP_DEAUTH_DETECT;
          break;
        }
        case WIFI_DISPLAY_PMKID: {
          wifiApp = WIFI_APP_PMKID;
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

void WIFIMenu::render() {
  switch (wifiApp) {
    case WIFI_APP_NONE: {
      display->text("wifi menu", 0);
      display->text("", 1);

      switch (wifiMenuState) {
        case WIFI_DISPLAY_DEAUTH: {
          display->centeredText("DEAUTH", 2);
          break;
        }
        case WIFI_DISPLAY_DEAUTH_DETECT: {
          display->centeredText("DEAUTH DETECT", 2);
          break;
        }
        case WIFI_DISPLAY_PMKID: {
          display->centeredText("PMKID", 2);
          break;
        }
        default:
          break;
      }

      display->text("", 3);
      break;
    }
    case WIFI_APP_DEAUTH: {
      DeauthApp::render();
      break;
    }
    case WIFI_APP_DEAUTH_DETECT: {
      DeauthDetectApp::render();
      break;
    }
    case WIFI_APP_PMKID: {
      PMKIDApp::render();
      break;
    }
    default:
      break;
  }
};
