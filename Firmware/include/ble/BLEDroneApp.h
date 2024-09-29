
#ifndef BLEDRONEAPP_H
#define BLEDRONEAPP_H

#include <math.h>
#include <time.h>
#include <sys/time.h>

#include "id_open.h"

#include <ArduinoJson.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"

#include "settings.h"
#include "Display.h"
#include "LEDStrip.h"
#include "ble/BLEMenu.h"
#include "wifi/WebServerModule.h"

extern LEDStrip* leds;
extern Display* display;
extern BLEApp bleApp;

enum DroneState {
  BLE_DRONE_STATE_HOME,
  BLE_DRONE_STATE_RUNNING,
};

struct BLEDroneWebEventParams {
  const char* action;
  const char* name;
  float lat;
  float lng;
};

class BLEDroneApp {
  private:
    static void _task(void *taskParams);

  public:
    static void sendUpdate();
    static void sendState(int state);
    static void sendOutput(String output);
    static void webEvent(BLEDroneWebEventParams *ps);
    static void stop();
    static void start(BLEDroneWebEventParams *ps);
    static void start(void* params);
    static void ontouch(int pad, const char* type, uint32_t value);
    static void render();
};

#endif // BLEDRONEAPP_H
