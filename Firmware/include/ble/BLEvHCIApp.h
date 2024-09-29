#ifndef BLEvHCIAPP_H
#define BLEvHCIAPP_H

#include <ArduinoJson.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_event.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_err.h"
#include "esp_bt.h"

#include "settings.h"
#include "Display.h"
#include "LEDStrip.h"
#include "ble/BLEMenu.h"
#include "wifi/WebServerModule.h"

extern LEDStrip* leds;
extern Display* display;
extern BLEApp bleApp;

enum BLEvHCIAppState {
  BLE_VHCI_STATE_HOME,
  BLE_VHCI_STATE_RUNNING,
};

struct BLEvHCIWebEventParams {
  const char* action;
  const char* hex_buffer;
};

class BLEvHCIApp {
  private:
    static void _task(void *taskParams);

  public:
    static void sendOutput(String output);
    static void sendState(int state);
    static void is_ready(void);
    static int on_pkt(uint8_t *data, uint16_t len);
    static void webEvent(BLEvHCIWebEventParams *ps);
    static void stop();
    static void start();
    static void ontouch(int pad, const char* type, uint32_t value);
    static void render();
};

#endif // BLEvHCIAPP_H
