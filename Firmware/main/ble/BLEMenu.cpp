#include "esp_log.h"

#include "Display.h"
#include "settings.h"
#include "LEDStrip.h"
#include "DisplayManager.h"
#include "ble/BLESpam.h"
#include "ble/BLEDroneApp.h"
#include "ble/BLESmartTagApp.h"
#include "ble/BLESkimmerDetectApp.h"
#include "ble/BLEFastPairSpamApp.h"
#include "ble/BLEEasySetupSpamApp.h"
#include "ble/GattAttackApp.h"
#include "ble/BLESpamDetectApp.h"
#include "ble/BLEvHCIApp.h"
#include "ble/BLESpamApp.h"
#include "ble/BLEReplayApp.h"
#include "ble/BLEMenu.h"

#define TAG "BLEMenu"

extern Display* display;
extern LEDStrip* leds;

BLEApp bleApp = BLE_APP_NONE;
static BLEMenuState bleMenuState = BLE_DISPLAY_GATTA;

void BLEMenu::setup() {};

void BLEMenu::nextState() {
  bleMenuState = static_cast<BLEMenuState>((bleMenuState + 1) % BLE_DISPLAY_COUNT);
}

void BLEMenu::prevState() {
  if (bleMenuState == BLE_DISPLAY_GATTA) {
    bleMenuState = static_cast<BLEMenuState>(BLE_DISPLAY_COUNT - 1);
  } else {
    bleMenuState = static_cast<BLEMenuState>(bleMenuState - 1);
  }
}

void BLEMenu::ontouch(int touchPadNum, const char* eventType, uint32_t touchValue) {
  if (bleApp == BLE_APP_GATTA) {
    GattAttackApp::ontouch(touchPadNum, eventType, touchValue);
    return;
  }

  if (bleApp == BLE_APP_SPAM) {
    BLENameSpamApp::ontouch(touchPadNum, eventType, touchValue);
    return;
  }

  if (bleApp == BLE_APP_FASTPAIR_SPAM) {
    BLEFastPairSpamApp::ontouch(touchPadNum, eventType, touchValue);
    return;
  }

  if (bleApp == BLE_APP_SPAM_DETECT) {
    BLESpamDetectApp::ontouch(touchPadNum, eventType, touchValue);
    return;
  }

  if (bleApp == BLE_APP_EASYSETUP_SPAM) {
    BLEEasySetupSpamApp::ontouch(touchPadNum, eventType, touchValue);
    return;
  }

  if (bleApp == BLE_APP_VHCI) {
    BLEvHCIApp::ontouch(touchPadNum, eventType, touchValue);
    return;
  }

  if (bleApp == BLE_APP_SKIMMER_DETECT) {
    BLESkimmerDetectApp::ontouch(touchPadNum, eventType, touchValue);
    return;
  }

  if (bleApp == BLE_APP_FAKE_DRONE) {
    BLEDroneApp::ontouch(touchPadNum, eventType, touchValue);
    return;
  }

  if (bleApp == BLE_APP_SMART_TAG) {
    BLESmartTagApp::ontouch(touchPadNum, eventType, touchValue);
    return;
  }

  switch (touchPadNum) {
    case LEFT: {
      BLEMenu::prevState();
      break;
    }
    case RIGHT: {
      BLEMenu::nextState();
      break;
    }
    case MIDDLE: {
      if (strcmp(eventType, "longPress") == 0) {
        if (bleApp == BLE_APP_NONE) {
          DisplayManager::setMenu(MAIN_MENU);
        } else {
          bleApp = BLE_APP_NONE;
          DisplayManager::appRunning = true;
        }
        return;
      }

      switch (bleMenuState) {
        case BLE_DISPLAY_SPAM: {
          bleApp = BLE_APP_SPAM;
          break;
        }
        case BLE_DISPLAY_GATTA: {
          bleApp = BLE_APP_GATTA;
          break;
        }
        case BLE_DISPLAY_FASTPAIR_SPAM: {
          bleApp = BLE_APP_FASTPAIR_SPAM;
          break;
        }
        case BLE_DISPLAY_EASYSETUP_SPAM: {
          bleApp = BLE_APP_EASYSETUP_SPAM;
          break;
        }
        case BLE_DISPLAY_VHCI: {
          bleApp = BLE_APP_VHCI;
          break;
        }
        case BLE_DISPLAY_SKIMMER_DETECT: {
          bleApp = BLE_APP_SKIMMER_DETECT;
          break;
        }
        case BLE_DISPLAY_FAKE_DRONE: {
          bleApp = BLE_APP_FAKE_DRONE;
          break;
        }
        case BLE_DISPLAY_SMART_TAG: {
          bleApp = BLE_APP_SMART_TAG;
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

void BLEMenu::render() {
  switch (bleApp) {
    case BLE_APP_NONE: {
      display->text("ble menu", 0);
      display->text("", 1);

      switch (bleMenuState) {
        case BLE_DISPLAY_GATTA: {
          display->centeredText("GATT ATTACK", 2);
          break;
        }
        case BLE_DISPLAY_SPAM: {
          display->centeredText("NAME SPAM", 2);
          break;
        }
        case BLE_DISPLAY_SPAM_DETECT: {
          display->centeredText("SPAM DETECT", 2);
          break;
        }
        case BLE_DISPLAY_FASTPAIR_SPAM: {
          display->centeredText("FASTPAIR SPAM", 2);
          break;
        }
        case BLE_DISPLAY_EASYSETUP_SPAM: {
          display->centeredText("EASYSETUP SPAM", 2);
          break;
        }
        case BLE_DISPLAY_VHCI: {
          display->centeredText("vHCI", 2);
          break;
        }
        case BLE_DISPLAY_SKIMMER_DETECT: {
          display->centeredText("SKIMMER DETECT", 2);
          break;
        }
        case BLE_DISPLAY_FAKE_DRONE: {
          display->centeredText("FAKE DRONE", 2);
          break;
        }
        case BLE_DISPLAY_SMART_TAG: {
          display->centeredText("SMART TAGGER", 2);
          break;
        }
        default:
          break;
      }

      display->text("", 3);
      break;
    }
    case BLE_APP_GATTA: {
      GattAttackApp::render();
      break;
    }
    case BLE_APP_SPAM: {
      BLENameSpamApp::render();
      break;
    }
    case BLE_APP_SPAM_DETECT: {
      BLESpamDetectApp::render();
      break;
    }
    case BLE_APP_FASTPAIR_SPAM: {
      BLEFastPairSpamApp::render();
      break;
    }
    case BLE_APP_EASYSETUP_SPAM: {
      BLEEasySetupSpamApp::render();
      break;
    }
    case BLE_APP_VHCI: {
      BLEvHCIApp::render();
      break;
    }
    case BLE_APP_SKIMMER_DETECT: {
      BLESkimmerDetectApp::render();
      break;
    }
    case BLE_APP_FAKE_DRONE: {
      BLEDroneApp::render();
      break;
    }
    case BLE_APP_SMART_TAG: {
      BLESmartTagApp::render();
      break;
    }
    default:
      break;
  }
};
