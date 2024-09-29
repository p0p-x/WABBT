#include "BLEDevice.h"

#include "ble/BLEvHCIApp.h"
#include "DisplayManager.h"

#define TAG "BLEvHCI"

static uint8_t hci_cmd_buf[128];

TaskHandle_t bleVHCITaskHandle = NULL;
BLEvHCIAppState bleVHCIState = BLE_VHCI_STATE_HOME;

void BLEvHCIApp::sendOutput(String output) {
  JsonDocument doc;
  String res;

  doc["app"] = APP_ID_BLE_VHCI;
  doc["output"] = output;
  serializeJson(doc, res);
  WebServerModule::notifyClients(res);
};

void BLEvHCIApp::sendState(int state) {
  JsonDocument doc;
  String res;

  doc["app"] = APP_ID_BLE_VHCI;
  doc["state"] = state;
  serializeJson(doc, res);
  WebServerModule::notifyClients(res);
};

void BLEvHCIApp::stop() {
  if (bleVHCITaskHandle != NULL) { 
    vTaskDelete(bleVHCITaskHandle);
    bleVHCITaskHandle = NULL;
  }

  BLEDevice::deinit();

  leds->blink(255, 0, 0, 2);
  bleVHCIState = BLE_VHCI_STATE_HOME;
  BLEvHCIApp::sendState((int)bleVHCIState);
};

void BLEvHCIApp::start() {
  esp_err_t ret = xTaskCreate(&BLEvHCIApp::_task, "blevhci", 16000, NULL, 1, &bleVHCITaskHandle);
  if (ret == ESP_FAIL) {
    ESP_LOGE(TAG, "blevhci task err: %s", esp_err_to_name(ret));
  }
  bleVHCIState = BLE_VHCI_STATE_RUNNING;
  BLEvHCIApp::sendState((int)bleVHCIState);
};

void BLEvHCIApp::is_ready(void) {
  ESP_LOGI(TAG, "Controller ready for pkt");
};

int BLEvHCIApp::on_pkt(uint8_t *data, uint16_t len) {
  char out[128];
  
  for (uint16_t i = 0; i < len; i++) {
    sprintf(&out[i], "%02x", data[i]);
  }
  
  ESP_LOGI(TAG, "PKT: %s", out);
  BLEvHCIApp::sendOutput(out);

  return 0;
};

void BLEvHCIApp::_task(void *taskParams) {
  BLEDevice::init("");
  
  static esp_vhci_host_callback_t vhci_host_cb = {
    BLEvHCIApp::is_ready,
    BLEvHCIApp::on_pkt
  };

  esp_vhci_host_register_callback(&vhci_host_cb);

  while (1) {
    vTaskDelay(20 / portTICK_PERIOD_MS);
  }
};

void BLEvHCIApp::webEvent(BLEvHCIWebEventParams *ps) {
  ESP_LOGI(TAG, "Web: %s", ps->action);
  JsonDocument doc;
  String res;

  if (strcmp(ps->action, "status") == 0) {
    doc["app"] = APP_ID_BLE_VHCI;
    doc["state"] = (int)bleVHCIState;
    serializeJson(doc, res);
    return WebServerModule::notifyClients(res);
  }

  if (strcmp(ps->action, "send") == 0) {
    if (ps->hex_buffer == nullptr) {
      doc["app"] = APP_ID_BLE_VHCI;
      doc["success"] = false;
      doc["error"] = "Invalid input buffer";

      serializeJson(doc, res);
      return WebServerModule::notifyClients(res);
    }
    
    size_t length = strlen(ps->hex_buffer) / 2;
    uint8_t buffer[length];

    for (size_t i = 0; i < length; i++) {
      char byteString[3] = { ps->hex_buffer[i * 2], ps->hex_buffer[i * 2 + 1], '\0' };
      buffer[i] = (uint8_t)strtoul(byteString, NULL, 16);
    }

    char out[2 * length + 1];
    for (size_t i = 0; i < length; i++) {
      sprintf(&out[i * 2], "%02x", buffer[i]);
    }
    out[2 * length] = '\0';

    ESP_LOGI(TAG, "INPUT: %s", out);

    return esp_vhci_host_send_packet(buffer, length);
  }

  if (strcmp(ps->action, "start") == 0) {
    DisplayManager::appRunning = true;
    leds->stop_animations();
    bleApp = BLE_APP_VHCI;
    DisplayManager::setMenu(BLE_MENU);
    BLEvHCIApp::start();
  }

  if (strcmp(ps->action, "stop") == 0) {
    BLEvHCIApp::stop();
    DisplayManager::appRunning = false;
    bleApp = BLE_APP_NONE;
    DisplayManager::setMenu(MAIN_MENU);
    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }
};

void BLEvHCIApp::ontouch(int pad, const char* type, uint32_t value) {
  switch (pad) {
    case LEFT: {
      break;
    }
    case RIGHT: {
      break;
    }
    case MIDDLE: {
      if (strcmp(type, "longPress") == 0) {
        switch (bleVHCIState) {
          case BLE_VHCI_STATE_HOME: {
            bleApp = BLE_APP_NONE;
            DisplayManager::appRunning = false;
            return;
          }
          case BLE_VHCI_STATE_RUNNING: {
            stop();
            return;
          }
          default:
            break;
        }
      }

      switch (bleVHCIState) {
        case BLE_VHCI_STATE_HOME: {
          start();
          break;
        }
        default:
          break;
      }
      
      break;
    }
  }
};

void BLEvHCIApp::render() {
  switch (bleVHCIState) {
    case BLE_VHCI_STATE_HOME: {
      display->centeredText("ble vhci", 0);
      display->text("", 1);
      display->centeredText("start", 2);
      display->text("", 3);
      break;
    }
    case BLE_VHCI_STATE_RUNNING: {
      display->centeredText("ble vhci", 0);
      display->text("", 1);
      display->centeredText("running", 2);
      display->centeredText("check web/serial", 3);
      break;
    }
    default:
      break;
  }

  vTaskDelay(20 / portTICK_PERIOD_MS);
};