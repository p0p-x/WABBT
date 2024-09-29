#ifndef BLESmartTagApp_H
#define BLESmartTagApp_H

#include <ArduinoJson.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_event.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_err.h"
#include "esp_gap_ble_api.h"
#include "esp_gattc_api.h"

#include "settings.h"
#include "Display.h"
#include "LEDStrip.h"
#include "ble/BLEMenu.h"
#include "wifi/WebServerModule.h"

extern LEDStrip* leds;
extern Display* display;
extern BLEApp bleApp;

enum BLESmartTagAppState {
  BLE_SMART_TAG_STATE_HOME,
  BLE_SMART_TAG_STATE_RUNNING,
};

struct BLESmartTagWebEventParams {
  const char* action;
};

class BLESmartTagApp {
  private:
    static void _task(void *taskParams);

  public:
    static void sendOutput(String output);
    static void sendState(int state);
    static void webEvent(BLESmartTagWebEventParams *ps);
    static void client_event(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);
    static void gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
    static void stop();
    static void start();
    static void ontouch(int pad, const char* type, uint32_t value);
    static void render();
};

#endif // BLESmartTagApp_H
