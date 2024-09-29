#ifndef BLEEASYSETUPSPAMAPP_H
#define BLEEASYSETUPSPAMAPP_H

#include <ArduinoJson.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_event.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_err.h"

#include "settings.h"
#include "Display.h"
#include "LEDStrip.h"
#include "ble/BLEMenu.h"
#include "wifi/WebServerModule.h"

extern LEDStrip* leds;
extern Display* display;
extern BLEApp bleApp;

enum BLEEasySetupSpamAppState {
  BLE_EASYSETUP_SPAM_STATE_HOME,
  BLE_EASYSETUP_SPAM_STATE_RUNNING,
};

struct BLEEasySetupSpamWebEventParams {
  const char* action;
};

class BLEEasySetupSpamApp {
  private:
    static void _task(void *taskParams);

  public:
    static void sendState(int state);
    static void webEvent(BLEEasySetupSpamWebEventParams *ps);
    static void stop_attack();
    static void start_attack();
    static void ontouch(int pad, const char* type, uint32_t value);
    static void render();
};

#endif // BLEEASYSETUPSPAM_H
