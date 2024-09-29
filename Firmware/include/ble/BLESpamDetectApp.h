#ifndef BLESpamDetectApp_H
#define BLESpamDetectApp_H

#include <ArduinoJson.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_event.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_err.h"
#include "esp_gap_ble_api.h"

#include "settings.h"
#include "Display.h"
#include "LEDStrip.h"
#include "ble/BLEMenu.h"
#include "wifi/WebServerModule.h"

extern LEDStrip* leds;
extern Display* display;
extern BLEApp bleApp;

enum BLESpamDetectAppState {
  BLE_SPAM_DETECT_STATE_HOME,
  BLE_SPAM_DETECT_STATE_RUNNING,
};

struct BLESpamDetectWebEventParams {
  const char* action;
};

class BLESpamDetectApp {
  private:
    static void _task(void *taskParams);

  public:
    static void sendOutput(String output);
    static void sendState(int state);
    static void webEvent(BLESpamDetectWebEventParams *ps);
    static void gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
    static void stop_detect();
    static void start_detect();
    static void ontouch(int pad, const char* type, uint32_t value);
    static void render();
};

#endif // BLEFASTPAIRSPAM_H
