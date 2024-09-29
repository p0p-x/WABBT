#ifndef DISPLAYMANAGER_H
#define DISPLAYMANAGER_H

enum MenuState {
  MAIN_MENU,
  BLE_MENU,
  WIFI_MENU,
  SETTINGS_MENU,
  MENU_STATE_COUNT,
};

enum MainMenuState {
  DISPLAY_NAME,
  DISPLAY_WIFI,
  DISPLAY_BLE,
  DISPLAY_SETTINGS,
  DISPLAY_STATE_COUNT
};

class DisplayManager {
public:
  static bool appRunning;
  static void setup();
  static void setMenu(MenuState menu);
  static void nextMenu();
  static void prevMenu();
  static void nextState();
  static void prevState();
  static void render();
};

#endif // DISPLAYMANAGER_H
