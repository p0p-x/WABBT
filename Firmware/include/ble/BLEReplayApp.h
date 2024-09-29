#ifndef REPLAYAPP_H
#define REPLAYAPP_H

#include <ArduinoJson.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_bt_defs.h"

#include "settings.h"
#include "Display.h"
#include "LEDStrip.h"
#include "ble/BLEMenu.h"

extern Display* display;
extern BLEApp bleApp;

enum BLEReplayState {
  BLE_REPLAY_STATE_HOME,
  BLE_REPLAY_STATE_SCANNING,
  BLE_REPLAY_STATE_SCAN_RES,
  BLE_REPLAY_STATE_START,
  BLE_REPLAY_STATE_RUNNING,
};

struct BLEReplayParams {
  uint8_t *target;
  JsonArray send;
};

struct BLEReplayWebEventParams {
  const char* action;
  int id;
  JsonArray send;
};

class BLEReplayApp {
  private:
    static void scan_task(void *taskParams);
    static void _task(void *taskParams);
    static void start_replay();

  public:
    static void sendState(int state);
    static void sendOutput(String output);
    static void webEvent(BLEReplayWebEventParams *ps);
    static void scan();
    static void scan(void* params);
    static void stop();
    static void start();
    static void gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
    static void client_event(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);
    static void ontouch(int pad, const char* type, uint32_t value);
    static void render();
};


#endif // REPLAYAPP_H
