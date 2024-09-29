#ifndef NAMESPAMAPP_H
#define NAMESPAMAPP_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"

#include "settings.h"
#include "Display.h"
#include "LEDStrip.h"
#include "ble/BLEMenu.h"
#include "ble/BLESpam.h"

extern LEDStrip* leds;
extern Display* display;
extern BLEApp bleApp;

enum BLENameSpamState {
  BLE_NAME_SPAM_STATE_HOME,
  BLE_NAME_SPAM_STATE_START,
  BLE_NAME_SPAM_STATE_RUNNING,
  BLE_NAME_SPAM_STATE_FAILED,
};

struct NameSpamParams {
  const char* name;
};

struct NameSpamWebEventParams {
  const char* action;
  const char* name;
};

class BLENameSpamApp {
  private:
    static void _task(void *taskParams);
    static void AnimationTask(void *params);

  public:
    static void webEvent(NameSpamWebEventParams *ps);
    static void stop_attack();
    static void start_attack(const char* name);
    static void ontouch(int pad, const char* type, uint32_t value);
    static void render();
};


#endif // NAMESPAMAPP_H
