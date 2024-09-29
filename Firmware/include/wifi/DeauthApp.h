#ifndef DEAUTHAPP_H
#define DEAUTHAPP_H

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

enum DeauthState {
  WIFI_DEAUTH_STATE_HOME,
  WIFI_DEAUTH_STATE_SCAN,
  WIFI_DEAUTH_STATE_SCANNING,
  WIFI_DEAUTH_STATE_SCANNING_STA,
  WIFI_DEAUTH_STATE_SELECT_TARGET,
  WIFI_DEAUTH_STATE_SELECT_AP_OR_STA,
  WIFI_DEAUTH_STATE_SELECT_STA_TARGET,
  WIFI_DEAUTH_STATE_START,
  WIFI_DEAUTH_STATE_RUNNING,
  WIFI_DEAUTH_STATE_PAUSED,
};

struct WIFIDeauthParams {
  uint8_t* apMac;
  uint8_t* stMac;
  uint8_t reason;
  uint8_t ch;
};

struct DeauthWebEventParams {
  const char* action;
  int ap_id;
  int sta_id;
  uint8_t reason;
};

class DeauthApp {
  private:
    static void _task(void *taskParams);
    static void AnimationTask(void *params);

  public:
    static void sendUpdate();
    static void sendState(int state);
    static void sendOutput(String output);
    static void webEvent(DeauthWebEventParams *ps);
    static void scan();
    static void scan(void* params);
    static void on_rx_packet(void *buf, wifi_promiscuous_pkt_type_t type);
    static bool send_packet(uint8_t* packet, uint16_t packetSize, uint8_t ch);
    static void stop_attack();
    static void start_attack(uint8_t* apMac, uint8_t* stMac, uint8_t reason, uint8_t ch);
    static void ontouch(int pad, const char* type, uint32_t value);
    static void render();
};

#endif // DEAUTHAPP_H
