#ifndef DEAUTHDETECTAPP_H
#define DEAUTHDETECTAPP_H

#include <ArduinoJson.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_event.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_err.h"

#include "settings.h"
#include "Display.h"
#include "LEDStrip.h"
#include "wifi/WIFIMenu.h"
#include "wifi/WebServerModule.h"

extern LEDStrip* leds;
extern Display* display;

enum DeauthDetectState {
  WIFI_DEAUTH_DETECT_STATE_HOME,
  WIFI_DEAUTH_DETECT_STATE_RUNNING,
};

struct WIFIDeauthDetectParams {};

struct DeauthDetectWebEventParams {
  const char* action;
};

class DeauthDetectApp {
  private:
    static void _task(void *taskParams);

  public:
    static void sendUpdate();
    static void sendState(int state);
    static void sendOutput(String output);
    static void webEvent(DeauthDetectWebEventParams *ps);
    static void on_rx_packet(void *buf, wifi_promiscuous_pkt_type_t type);
    static void stop_detect();
    static void start_detect();
    static void start_detect(void* params);
    static void ontouch(int pad, const char* type, uint32_t value);
    static void render();
};

#endif // DEAUTHDETECTAPP_H
