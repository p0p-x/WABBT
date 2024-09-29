#ifndef WIFIMENU_H
#define WIFIMENU_H

enum WIFIApp {
  WIFI_APP_NONE,
  WIFI_APP_DEAUTH,
  WIFI_APP_DEAUTH_DETECT,
  WIFI_APP_PMKID,
};

enum WIFIMenuState {
  WIFI_DISPLAY_DEAUTH,
  WIFI_DISPLAY_DEAUTH_DETECT,
  WIFI_DISPLAY_PMKID,
  WIFI_DISPLAY_COUNT,
};

class WIFIMenu {
public:
  static void setup();
  static void render();
  static void nextState();
  static void prevState();
  static void ontouch(int pad, const char* type, uint32_t value);
};

#endif // WIFIMENU_H
