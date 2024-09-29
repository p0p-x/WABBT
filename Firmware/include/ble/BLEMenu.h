#ifndef BLEMENU_H
#define BLEMENU_H

enum BLEApp {
  BLE_APP_NONE,
  BLE_APP_GATTA,
  BLE_APP_SPAM,
  BLE_APP_SPAM_DETECT,
  BLE_APP_FASTPAIR_SPAM,
  BLE_APP_VHCI,
  BLE_APP_EASYSETUP_SPAM,
  BLE_APP_SKIMMER_DETECT,
  BLE_APP_FAKE_DRONE,
  BLE_APP_SMART_TAG,
  BLE_APP_REPLAY,
};

enum BLEMenuState {
  BLE_DISPLAY_GATTA,
  BLE_DISPLAY_SPAM,
  BLE_DISPLAY_SPAM_DETECT,
  BLE_DISPLAY_FASTPAIR_SPAM,
  BLE_DISPLAY_EASYSETUP_SPAM,
  BLE_DISPLAY_VHCI,
  BLE_DISPLAY_SKIMMER_DETECT,
  BLE_DISPLAY_FAKE_DRONE,
  BLE_DISPLAY_SMART_TAG,
  BLE_DISPLAY_COUNT
};

class BLEMenu {
public:
  static void setup();
  static void render();
  static void nextState();
  static void prevState();
  static void ontouch(int touchPadNum, const char* eventType, uint32_t touchValue);
};

#endif // BLEMENU_H
