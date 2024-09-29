#include <BLEDevice.h>
#include <ArduinoJson.h>
#include "esp_err.h"
#include "nvs.h"
#include <nvs_flash.h>

#include "ble/GyattAttack.h"
#include "ble/GattAttackApp.h"
#include "wifi/WebServerModule.h"
#include "DisplayManager.h"

#define TAG "GattAttackApp"

extern LEDStrip* leds;
TaskHandle_t gattTaskHandle = NULL;
GattAttackState gattAttackState = GATT_ATTACK_STATE_HOME;

BLEScanResults* scanResults;
static uint32_t shownDeviceIndex = 0;

void GattAttackApp::scan(void* params) {
  GattAttackApp::scan();
  vTaskDelete(NULL);
};

void GattAttackApp::sendOutput(String output) {
  ESP_LOGI(TAG, "%s", output.c_str());
  JsonDocument doc;
  String res;

  doc["app"] = APP_ID_BLE_GATTATTACK;
  doc["output"] = output;
  serializeJson(doc, res);
  WebServerModule::notifyClients(res);
};

void GattAttackApp::sendState(int state) {
  JsonDocument doc;
  String res;

  doc["app"] = APP_ID_BLE_GATTATTACK;
  doc["state"] = state;
  serializeJson(doc, res);
  WebServerModule::notifyClients(res);
};

void GattAttackApp::scan() {
  shownDeviceIndex = 0;
  gattAttackState = GATT_ATTACK_STATE_SCANNING;

  // scan for devices
  BLEDevice::init("");
  BLEScan* bleScan = BLEDevice::getScan();
  bleScan->setActiveScan(true);
  scanResults = bleScan->start(10);

  // clean up bt stack and prep it for a new init()
  stop_gatt_attack(); // too lazy to write a function to tear down bt stack
  BLEDevice::deinit(); // need to tear down BLEDevice init flags/etc

  ESP_LOGI(TAG, "Scan Finished. Got %i Devices", scanResults->getCount());

  JsonDocument doc;
  String res;

  doc["app"] = APP_ID_BLE_GATTATTACK;
  doc["scan_done"] = true;
  serializeJson(doc, res);
  WebServerModule::notifyClients(res);

  gattAttackState = GATT_ATTACK_STATE_SCAN_RES;
  GattAttackApp::sendState((int)gattAttackState);
};

void GattAttackApp::stop_attack() {
  stop_gatt_attack();

  if (gattTaskHandle != NULL) { 
    vTaskDelete(gattTaskHandle);
    gattTaskHandle = NULL;
  }

  gattAttackState = GATT_ATTACK_STATE_HOME;
  GattAttackApp::sendState((int)gattAttackState);
};

void GattAttackApp::start_attack(esp_bd_addr_t target) {
  GattAttackParams* params = new GattAttackParams{target};
  esp_err_t ret = xTaskCreate(&GattAttackApp::atk_task, "gatk", 70000, params, 0, &gattTaskHandle);
  if (ret == ESP_FAIL) {
    esp_err_t ret = xTaskCreate(&GattAttackApp::atk_task, "gatk", 16000, params, 0, &gattTaskHandle);
    if (ret == ESP_FAIL) {
      ESP_LOGE(TAG, "#2 gatk task err: %s", esp_err_to_name(ret));
    }
  }
  gattAttackState = GATT_ATTACK_STATE_START;
  GattAttackApp::sendState((int)gattAttackState);
};

void GattAttackApp::atk_task(void *taskParams) {
  GattAttackParams *params = static_cast<GattAttackParams*>(taskParams);
  start_gatt_attack(params->target);
  while(1) {
    vTaskDelay(20 / portTICK_PERIOD_MS);
  }
};

void GattAttackApp::webEvent(GattAttackWebEventParams *ps) {
  ESP_LOGI(TAG, "Web: %s", ps->action);
  JsonDocument doc;
  String res;

  if (strcmp(ps->action, "status") == 0) {
    doc["app"] = APP_ID_BLE_GATTATTACK;
    doc["state"] = (int)gattAttackState;
    serializeJson(doc, res);
    return WebServerModule::notifyClients(res);
  }

  if (strcmp(ps->action, "start") == 0) {
    DisplayManager::appRunning = true;
    leds->stop_animations();
    bleApp = BLE_APP_GATTA;
    DisplayManager::setMenu(BLE_MENU);
    BLEAddress bleAddr = scanResults->getDevice(ps->id).getAddress();
    esp_bd_addr_t target;
    memcpy(target, bleAddr.getNative(), 6);
    GattAttackApp::start_attack(target);
  }

  if (strcmp(ps->action, "stop") == 0) {
    GattAttackApp::stop_attack();
    DisplayManager::appRunning = false;
    bleApp = BLE_APP_NONE;
    DisplayManager::setMenu(MAIN_MENU);
  }

  if (strcmp(ps->action, "scan") == 0) {
    DisplayManager::appRunning = true;
    leds->stop_animations();
    bleApp = BLE_APP_GATTA;
    DisplayManager::setMenu(BLE_MENU);
    xTaskCreate(&GattAttackApp::scan, "gatk", 4000, NULL, 0, NULL);
  }

  if (strcmp(ps->action, "results") == 0) {
    JsonArray data = doc["results"].to<JsonArray>();
    
    for (int i = 0; i < scanResults->getCount(); i++) {
      BLEAdvertisedDevice advertisedDevice = scanResults->getDevice(i);
      JsonObject obj = data.add<JsonObject>();
      String macAddress = advertisedDevice.getAddress().toString();
      String deviceName = advertisedDevice.getName();
      int rssi = advertisedDevice.getRSSI();
      obj["id"] = i;
      obj["mac"] = macAddress;
      obj["name"] = deviceName.length() > 0 ? deviceName : "";
      obj["rssi"] = rssi;
    }

    doc["app"] = APP_ID_BLE_GATTATTACK;
    serializeJson(doc, res);
    return WebServerModule::notifyClients(res);
  }
};

void GattAttackApp::ontouch(int pad, const char* type, uint32_t value) {
  switch (pad) {
    case LEFT: {
      if (gattAttackState == GATT_ATTACK_STATE_SCAN_RES) {
        if (shownDeviceIndex == 0) {
          shownDeviceIndex = scanResults->getCount() - 1;
        } else {
          shownDeviceIndex -= 1;
        }

        return;
      }
      break;
    }
    case RIGHT: {
      if (gattAttackState == GATT_ATTACK_STATE_SCAN_RES) {
        if ((shownDeviceIndex + 1) == scanResults->getCount()) {
          shownDeviceIndex = 0;
        } else {
          shownDeviceIndex += 1;
        }

        return;
      }
      break;
    }
    case MIDDLE: {
      if (strcmp(type, "longPress") == 0) {
        if (gattAttackState == GATT_ATTACK_STATE_HOME) {
          bleApp = BLE_APP_NONE;
          DisplayManager::appRunning = false;
          return;
        }

        switch (gattAttackState) {
          case GATT_ATTACK_STATE_SCAN_RES: {
            gattAttackState = GATT_ATTACK_STATE_HOME;
            GattAttackApp::sendState((int)gattAttackState);
            return;
          }
          case GATT_ATTACK_STATE_START:
          case GATT_ATTACK_STATE_FAILED:
          case GATT_ATTACK_STATE_CLIENT_CONNECTED:
          case GATT_ATTACK_STATE_CLIENT_CONNECT_FAIL:
          case GATT_ATTACK_STATE_RUNNING: {
            stop_attack();
            return;
          }
          case GATT_ATTACK_STATE_CLIENT_CONNECTING: {
            ESP_LOGI(TAG, "Prevent kill gatt attck during client connection period");
            GattAttackApp::sendOutput("Cant kill while client connecting due to crash bug");
            return;
          }
          default:
            break;
        }
      }

      if (gattAttackState == GATT_ATTACK_STATE_HOME) {
        // Scan for BLE devices
        GattAttackApp::scan();
        return;
      }

      if (gattAttackState == GATT_ATTACK_STATE_SCAN_RES) {
        // Select Target Mac
        BLEAddress bleAddr = scanResults->getDevice(shownDeviceIndex).getAddress();
        esp_bd_addr_t target;

        // Copy BLE address to target
        memcpy(target, bleAddr.getNative(), 6);
        GattAttackApp::start_attack(target);
        return;
      }
      
      break;
    }
  }
};

void GattAttackApp::render() {

  switch (gattAttackState) {
    case GATT_ATTACK_STATE_HOME: {
      display->text("gatt attack", 0);
      display->text("", 1);
      display->centeredText("scan", 2);
      display->text("", 3);
      break;
    }
    case GATT_ATTACK_STATE_SCANNING: {
      display->text("gatt attack", 0);
      display->text("", 1);
      display->centeredText("scanning", 2);
      display->text("", 3);
      break;
    }
    case GATT_ATTACK_STATE_SCAN_RES: {
      char line[24];
      BLEAdvertisedDevice dev = scanResults->getDevice(shownDeviceIndex);
      snprintf(line, sizeof(line), "%" PRIu32 "/%i",
              shownDeviceIndex+1,
              scanResults->getCount()
              );
      display->centeredText(line, 0);
      display->text(dev.getName().c_str(), 1);
      display->text(dev.getAddress().toString().c_str(), 2);
      snprintf(line, sizeof(line), "RSSI: %i", dev.getRSSI());
      display->text(line, 3);
      break;
    }
    case GATT_ATTACK_STATE_START: {
      display->text("gatt attack", 0);
      display->text("", 1);
      display->centeredText("starting", 2);
      display->text("", 3);
      break;
    }
    case GATT_ATTACK_STATE_CLIENT_CONNECT_FAIL:
    case GATT_ATTACK_STATE_CLIENT_CONNECTING: {
      display->text("gatt attack", 0);
      display->text("", 1);
      display->centeredText("connecting", 2);
      display->text("", 3);
      break;
    }
    case GATT_ATTACK_STATE_CLIENT_CONNECTED: {
      display->text("gatt attack", 0);
      display->text("", 1);
      display->centeredText("get services", 2);
      display->text("", 3);
      break;
    }
    case GATT_ATTACK_STATE_RUNNING: {
      display->text("gatt attack", 0);
      display->text("", 1);
      display->centeredText("Running", 2);
      display->text("", 3);
      break;
    }
    case GATT_ATTACK_STATE_FAILED: {
      display->text("gatt attack", 0);
      display->text("", 1);
      display->centeredText("Failed :(", 2);
      display->text("", 3);
      break;
    }
    default:
      break;
  }

  vTaskDelay(60 / portTICK_PERIOD_MS);
};