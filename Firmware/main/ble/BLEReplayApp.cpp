#include <BLEDevice.h>
#include "esp_err.h"

#include "wifi/WebServerModule.h"
#include "ble/BLEReplayApp.h"
#include "ble/GyattAttack.h"
#include "DisplayManager.h"

#define TAG "ReplayApp"

extern LEDStrip* leds;
TaskHandle_t replayTaskHandle = NULL;
BLEReplayState replayState = BLE_REPLAY_STATE_HOME;

bool connecting = false;
bool connected = false;
BLEScanResults* scanResults;
static uint32_t shownDeviceIndex = 0;
static esp_gatt_if_t interface = ESP_GATT_IF_NONE;
static esp_bd_addr_t target;
static JsonArray packets;

void BLEReplayApp::sendOutput(String output) {
  ESP_LOGI(TAG, "%s", output.c_str());
  JsonDocument doc;
  String res;

  doc["app"] = APP_ID_BLE_REPLAY;
  doc["output"] = output;
  serializeJson(doc, res);
  WebServerModule::notifyClients(res);
};

void BLEReplayApp::sendState(int state) {
  JsonDocument doc;
  String res;

  doc["app"] = APP_ID_BLE_REPLAY;
  doc["state"] = state;
  serializeJson(doc, res);
  WebServerModule::notifyClients(res);
};

void BLEReplayApp::scan() {
  shownDeviceIndex = 0;
  replayState = BLE_REPLAY_STATE_SCANNING;
  BLEReplayApp::sendState((int)replayState);

  // scan for devices
  BLEDevice::init("");
  BLEScan* bleScan = BLEDevice::getScan();
  bleScan->setActiveScan(true);
  scanResults = bleScan->start(10);

  // clean up bt stack and prep it for a new init()
  BLEDevice::deinit(); // need to tear down BLEDevice init flags/etc

  ESP_LOGI(TAG, "Scan Finished. Got %i Devices", scanResults->getCount());

  JsonDocument doc;
  String res;

  doc["app"] = APP_ID_BLE_REPLAY;
  doc["scan_done"] = true;
  serializeJson(doc, res);
  WebServerModule::notifyClients(res);

  replayState = BLE_REPLAY_STATE_SCAN_RES;
  BLEReplayApp::sendState((int)replayState);
};

void BLEReplayApp::scan(void* params) {
  BLEReplayApp::scan();
  vTaskDelete(NULL);
};

void BLEReplayApp::stop() {
  if (replayTaskHandle != NULL) { 
    vTaskDelete(replayTaskHandle);
    replayTaskHandle = NULL;
  }

  BLEDevice::deinit();

  replayState = BLE_REPLAY_STATE_HOME;
  BLEReplayApp::sendState((int)replayState);
};

void BLEReplayApp::start() {
  esp_err_t ret = xTaskCreate(&BLEReplayApp::_task, "replay", 70000, NULL, 0, &replayTaskHandle);
  if (ret == ESP_FAIL) {
    esp_err_t ret = xTaskCreate(&BLEReplayApp::_task, "replay", 16000, NULL, 0, &replayTaskHandle);
    if (ret == ESP_FAIL) {
      ESP_LOGE(TAG, "#2 replay task err: %s", esp_err_to_name(ret));
    }
  }
  replayState = BLE_REPLAY_STATE_RUNNING;
  BLEReplayApp::sendState((int)replayState);
};

void BLEReplayApp::_task(void *taskParams) {
  BLEDevice::init("");

  esp_err_t ret = esp_ble_gap_register_callback(BLEReplayApp::gap_cb);
  if (ret) {
    ESP_LOGE(TAG, "%s gap register failed, error code = %x", __func__, ret);
    return;
  }

  esp_ble_gap_set_scan_params(&default_scan_params);

  ret = esp_ble_gattc_register_callback(BLEReplayApp::client_event);
  if (ret) {
    ESP_LOGE(TAG, "%s gattc register failed, error code = %x", __func__, ret);
    return;
  }

  ret = esp_ble_gattc_app_register(0);
  if (ret) {
    ESP_LOGE(TAG, "%s gattc app register failed, error code = %x", __func__, ret);
  }
  while(1) {}
};

/**
 * start_replay
 *
 * Actually start sending the passed in
 * JSON packet structure.
 *
 * Called after connected to target
 * and services are discovered.
 */
void BLEReplayApp::start_replay() {
  String output;
  serializeJson(packets, output);

  sendOutput(output);

  for (JsonObject p : packets) {
    char prt[120];
    uint16_t handle = p["handle"];
    int delay = p["delay"];
    const char* hex_buffer = p["bytes"];
    size_t length = strlen(hex_buffer) / 2;
    uint8_t buffer[length];

    for (size_t i = 0; i < length; i++) {
      char byteString[3] = {
        hex_buffer[i * 2],
        hex_buffer[i * 2 + 1],
        '\0'
      };
      buffer[i] = (uint8_t)strtoul(byteString, NULL, 16);
    }

    char out[2 * length + 1];
    for (size_t i = 0; i < length; i++) {
      sprintf(&out[i * 2], "%02x", buffer[i]);
    }
    out[2 * length] = '\0';

    sprintf(prt, "handle: 0x%02x delay: %i bytes: %s",
            handle, delay, out);

    sendOutput(prt);
  }
};

void BLEReplayApp::gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
  switch (event) {
    case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT: {
      esp_ble_gap_start_scanning(0);
      break;
    }
    case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT: {
      if (param->scan_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
        ESP_LOGE(TAG, "scan start failed, error status = %x", param->scan_start_cmpl.status);
        break;
      }
      break;
    }
    case ESP_GAP_BLE_SCAN_RESULT_EVT: {
      switch (param->scan_rst.search_evt) {
        case ESP_GAP_SEARCH_INQ_RES_EVT: {
          if (connecting || connected) {
            break;
          }

          if (memcmp(param->scan_rst.bda, target, 6) == 0) {
            if (!connecting && !connected) {
              connecting = true;
              BLEReplayApp::sendOutput("Connecting");
              esp_ble_gattc_open(interface, target, param->scan_rst.ble_addr_type, true);
              esp_ble_gap_stop_scanning();
            }
          }
          break;
        }
        default:
          break;
      }
      
      break;
    }
    default:
      break;
  }
};

void BLEReplayApp::client_event(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param) {
  switch (event) {
    case ESP_GATTC_REG_EVT: {
      interface = gattc_if;
      break;
    }
    case ESP_GATTC_CONNECT_EVT: {
      connecting = false;
      connected = true;
      sendOutput("Connected");
      break;
    }
    case ESP_GATTC_DISCONNECT_EVT: {
      char out[150];
      connecting = false;
      connected = false;
      sprintf(out,"CLIENT DISCONNECT: reason 0x%x (%i) conn_id: %x - %02x:%02x:%02x:%02x:%02x:%02x",
              param->disconnect.reason, param->disconnect.reason,
              param->disconnect.conn_id,
              param->disconnect.remote_bda[0], param->disconnect.remote_bda[1], param->disconnect.remote_bda[2],
              param->disconnect.remote_bda[3], param->disconnect.remote_bda[4], param->disconnect.remote_bda[5]);
      sendOutput(out);
      break;
    }
    case ESP_GATTC_DIS_SRVC_CMPL_EVT: {
      sendOutput("Services Discovered. Starting Replay");
      uint8_t val[4] = { 0x01, 0x02, 0x03, 0x04 };
      esp_ble_gattc_write_char(interface, param->dis_srvc_cmpl.conn_id,
                              0x01, sizeof(val), val,
                              ESP_GATT_WRITE_TYPE_NO_RSP,
                              ESP_GATT_AUTH_REQ_NONE);
      start_replay();
      break;
    }
    case ESP_GATTC_SEARCH_CMPL_EVT: {
      break;
    }
    case ESP_GATTC_READ_DESCR_EVT: {
      char out[158 + param->read.value_len];
      char* val = bytes_to_hex_string(param->read.value, param->read.value_len);
      snprintf(out, sizeof(out), "READ DESC RSP: if: %i conn_id: %x status: %x handle: 0x%02x\n%s",
              gattc_if,
              param->read.conn_id,
              param->read.status,
              param->read.handle,
              val);
      sendOutput(out);
      free(val);
      break;
    }
    case ESP_GATTC_WRITE_DESCR_EVT: {
      char out[158];
      snprintf(out, sizeof(out), "WRITE DESC RSP: if: %i conn_id: %x status: %x handle: 0x%02x",
              gattc_if,
              param->write.conn_id,
              param->write.status,
              param->write.handle);
      sendOutput(out);
      break;
    }
    case ESP_GATTC_READ_CHAR_EVT: {
      char out[158 + param->read.value_len];
      char* val = bytes_to_hex_string(param->read.value, param->read.value_len);
      snprintf(out, sizeof(out), "READ CHAR RSP: if: %i conn_id: %x status: %x handle: 0x%02x\n%s",
              gattc_if,
              param->read.conn_id,
              param->read.status,
              param->read.handle,
              val);
      sendOutput(out);
      free(val);
      break;
    }
    case ESP_GATTC_WRITE_CHAR_EVT: {
      char out[200];
      snprintf(out, sizeof(out), "WRITE CHAR RSP: if: %i conn_id: %x status: %i handle: 0x%02x",
              gattc_if,
              param->write.conn_id,
              param->write.status,
              param->write.handle);
      sendOutput(out);
      break;
    }
    case ESP_GATTC_NOTIFY_EVT: {
      int sizeOut = 75 + param->notify.value_len;
      char out[sizeOut];
      char* value = bytes_to_hex_string(param->notify.value, param->notify.value_len);

      sprintf(out, "%s RSP: if: %x conn_id: %x handle: 0x%02x\n%s",
              param->notify.is_notify ? "NOTIFY" : "INDICATE",
              gattc_if,
              param->notify.conn_id,
              param->notify.handle,
              value);
      sendOutput(out);
      free(value);
      break;
    }
    default:
      break;
  }
};

void BLEReplayApp::webEvent(BLEReplayWebEventParams *ps) {
  ESP_LOGI(TAG, "Web: %s", ps->action);
  JsonDocument doc;
  String res;

  if (strcmp(ps->action, "status") == 0) {
    doc["app"] = APP_ID_BLE_REPLAY;
    doc["state"] = (int)replayState;
    serializeJson(doc, res);
    return WebServerModule::notifyClients(res);
  }

  if (strcmp(ps->action, "start") == 0) {
    DisplayManager::appRunning = true;
    leds->stop_animations();
    bleApp = BLE_APP_REPLAY;
    DisplayManager::setMenu(BLE_MENU);
    BLEAddress bleAddr = scanResults->getDevice(ps->id).getAddress();
    memcpy(target, bleAddr.getNative(), 6);
    packets = ps->send;
    BLEReplayApp::start();
  }

  if (strcmp(ps->action, "stop") == 0) {
    BLEReplayApp::stop();
    DisplayManager::appRunning = false;
    bleApp = BLE_APP_NONE;
    DisplayManager::setMenu(MAIN_MENU);
  }

  if (strcmp(ps->action, "scan") == 0) {
    DisplayManager::appRunning = true;
    leds->stop_animations();
    bleApp = BLE_APP_REPLAY;
    DisplayManager::setMenu(BLE_MENU);
    xTaskCreate(&BLEReplayApp::scan, "replay", 4000, NULL, 0, NULL);
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

    doc["app"] = APP_ID_BLE_REPLAY;
    serializeJson(doc, res);
    return WebServerModule::notifyClients(res);
  }
};

void BLEReplayApp::ontouch(int pad, const char* type, uint32_t value) {
  switch (pad) {
    case LEFT: {
      if (replayState == BLE_REPLAY_STATE_SCAN_RES) {
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
      if (replayState == BLE_REPLAY_STATE_SCAN_RES) {
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
        if (replayState == BLE_REPLAY_STATE_HOME) {
          bleApp = BLE_APP_NONE;
          DisplayManager::appRunning = false;
          return;
        }

        switch (replayState) {
          case BLE_REPLAY_STATE_SCAN_RES: {
            replayState = BLE_REPLAY_STATE_HOME;
            BLEReplayApp::sendState((int)replayState);
            return;
          }
          case BLE_REPLAY_STATE_RUNNING: {
            stop();
            return;
          }
          default:
            break;
        }
      }

      if (replayState == BLE_REPLAY_STATE_HOME) {
        // Scan for BLE devices
        BLEReplayApp::scan();
        return;
      }

      if (replayState == BLE_REPLAY_STATE_SCAN_RES) {
        // Select Target Mac & Start
        BLEAddress bleAddr = scanResults->getDevice(shownDeviceIndex).getAddress();
        memcpy(target, bleAddr.getNative(), 6);
        BLEReplayApp::start();
        return;
      }

      break;
    }
  }
};

void BLEReplayApp::render() {
  switch (replayState) {
    case BLE_REPLAY_STATE_HOME: {
      display->text("ble replay", 0);
      display->text("", 1);
      display->centeredText("scan", 2);
      display->text("", 3);
      break;
    }
    case BLE_REPLAY_STATE_SCANNING: {
      display->text("ble replay", 0);
      display->text("", 1);
      display->centeredText("scanning", 2);
      display->text("", 3);
      break;
    }
    case BLE_REPLAY_STATE_SCAN_RES: {
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
    case BLE_REPLAY_STATE_RUNNING: {
      display->text("ble replay", 0);
      display->text("", 1);
      display->centeredText("Running", 2);
      display->text("", 3);
      break;
    }
    default:
      break;
  }

  vTaskDelay(60 / portTICK_PERIOD_MS);
};