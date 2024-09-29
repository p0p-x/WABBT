#ifndef GATTATTACKAPP_H
#define GATTATTACKAPP_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "settings.h"
#include "Display.h"
#include "LEDStrip.h"
#include "ble/BLEMenu.h"
#include "ble/GyattAttack.h"

extern Display* display;
extern BLEApp bleApp;

enum GattAttackState {
  GATT_ATTACK_STATE_HOME,
  GATT_ATTACK_STATE_SCANNING,
  GATT_ATTACK_STATE_SCAN_RES,
  GATT_ATTACK_STATE_START,
  GATT_ATTACK_STATE_CLIENT_CONNECTING, // dont want to allow killing during this period to prevent crash
  GATT_ATTACK_STATE_CLIENT_CONNECT_FAIL,
  GATT_ATTACK_STATE_CLIENT_CONNECTED,
  GATT_ATTACK_STATE_RUNNING,
  GATT_ATTACK_STATE_FAILED,
};

struct GattAttackParams {
  uint8_t *target;
};

struct GattAttackWebEventParams {
  const char* action;
  int id;
};

class GattAttackApp {
  private:
    static void scan_task(void *taskParams);
    static void atk_task(void *taskParams);

  public:
    static void sendState(int state);
    static void sendOutput(String output);
    static void webEvent(GattAttackWebEventParams *ps);
    static bool isRunning;
    static void scan();
    static void scan(void* params);
    static void stop_attack();
    static void start_attack(esp_bd_addr_t bd_target);
    static void ontouch(int pad, const char* type, uint32_t value);
    static void render();
};


#endif // GATTATTACKAPP_H
