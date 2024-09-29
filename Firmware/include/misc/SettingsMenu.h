#ifndef SETTINGSMENU_H
#define SETTINGSMENU_H

enum SettingsApp {
  SETTINGS_APP_NONE,
  SETTINGS_APP_WIFI,
  SETTINGS_APP_TOUCH,
};

enum SettingsMenuState {
  SETTINGS_DISPLAY_WIFI,
  SETTINGS_DISPLAY_TOUCH,
  SETTINGS_DISPLAY_COUNT,
};

class SettingsMenu {
public:
  static void setup();
  static void render();
  static void nextState();
  static void prevState();
  static void ontouch(int touchPadNum, const char* eventType, uint32_t touchValue);
};

#endif // BLEMENU_H
