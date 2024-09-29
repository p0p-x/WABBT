#include "BLEDevice.h"

#include "ble/BLESpamDetectApp.h"
#include "DisplayManager.h"

#define TAG "BLESpamDetect"

int ble_spam_detect_last_led = 0;

TaskHandle_t bleSpamDetectTaskHandle = NULL;
BLESpamDetectAppState bleSpamDetectState = BLE_SPAM_DETECT_STATE_HOME;

uint8_t fastPairAdv[8] = {
  0x03, 0x03, 0x2C, 0xFE,
  0x06, 0x16, 0x2C, 0xFE
};

typedef enum {
  ContinuityTypeAirDrop = 0x05,
  ContinuityTypeProximityPair = 0x07,
  ContinuityTypeAirplayTarget = 0x09,
  ContinuityTypeHandoff = 0x0C,
  ContinuityTypeTetheringSource = 0x0E,
  ContinuityTypeNearbyAction = 0x0F,
  ContinuityTypeNearbyInfo = 0x10,
  ContinuityTypeCustomCrash,
  ContinuityTypeCOUNT
} ContinuityType;

uint8_t continuityAdv[3] = {
  0xFF, 0x4C, 0x00
};

uint8_t easySetupAdv[14] = {
  27, 0xFF, 0x75, 0x00,
  0x42, 0x09, 0x81, 0x02,
  0x14, 0x15, 0x03, 0x21,
  0x01, 0x09
};

uint8_t loveSpouseAdv[11] = {
  0xFF, 0xFF, 0x00, 0x6D,
  0xB6, 0x43, 0xCE, 0x97,
  0xFE, 0x42, 0x7C
};

uint8_t swiftPairAdv[6] = {
  0xFF, 0x06, 0x00, 0x03,
  0x00, 0x80
};

esp_ble_scan_params_t ble_detect_scan_params = {
  .scan_type = BLE_SCAN_TYPE_PASSIVE,
  .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
  .scan_filter_policy = BLE_SCAN_FILTER_ALLOW_ALL,
  .scan_interval = 0x50,
  .scan_window = 0x30,
  .scan_duplicate = BLE_SCAN_DUPLICATE_DISABLE
};

void BLESpamDetectApp::sendOutput(String output) {
  JsonDocument doc;
  String res;

  doc["app"] = APP_ID_BLE_SPAM_DETECT;
  doc["output"] = output;
  serializeJson(doc, res);
  WebServerModule::notifyClients(res);
};

void BLESpamDetectApp::sendState(int state) {
  JsonDocument doc;
  String res;

  doc["app"] = APP_ID_BLE_SPAM_DETECT;
  doc["state"] = state;
  serializeJson(doc, res);
  WebServerModule::notifyClients(res);
};

void BLESpamDetectApp::gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
  uint8_t *adv_name = NULL;
  uint8_t adv_name_len = 0;
  switch (event) {
    case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT: {
      uint32_t duration = 0;
      esp_ble_gap_start_scanning(duration);
      break;
    }
    case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT: {
      if (param->scan_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
        ESP_LOGE(TAG, "scan start failed, error status = %x", param->scan_start_cmpl.status);
        break;
      }
      ESP_LOGI(TAG, "SCAN START");

      break;
    }
    case ESP_GAP_BLE_SCAN_RESULT_EVT: {
      switch (param->scan_rst.search_evt) {
        case ESP_GAP_SEARCH_INQ_RES_EVT: {
          adv_name = esp_ble_resolve_adv_data(param->scan_rst.ble_adv,
                                              ESP_BLE_AD_TYPE_NAME_CMPL,
                                              &adv_name_len);

          char out[100];

          bool found = false;
          for (size_t i = 0; i <= param->scan_rst.adv_data_len; i++) {
            if (memcmp(&param->scan_rst.ble_adv[i], fastPairAdv, sizeof(fastPairAdv)) == 0) {
              found = true;
              sprintf(out, "FOUND FASTPAIR PACKET: %02x:%02x:%02x:%02x:%02x:%02x\t%i db",
                      param->scan_rst.bda[0], param->scan_rst.bda[1],
                      param->scan_rst.bda[2], param->scan_rst.bda[3],
                      param->scan_rst.bda[4], param->scan_rst.bda[5],
                      param->scan_rst.rssi);
            }

            if (memcmp(&param->scan_rst.ble_adv[i], continuityAdv, sizeof(continuityAdv)) == 0) {
              ContinuityType type = (ContinuityType)param->scan_rst.ble_adv[i + sizeof(continuityAdv)];
              // Currently not seeing flippers main ble spam repos using any other type.
              if (type == ContinuityTypeProximityPair || type == ContinuityTypeNearbyAction) {
                found = true;
                sprintf(out, "FOUND CONTINUITY PACKET: %02x - %02x:%02x:%02x:%02x:%02x:%02x\t%i db",
                        type,
                        param->scan_rst.bda[0], param->scan_rst.bda[1],
                        param->scan_rst.bda[2], param->scan_rst.bda[3],
                        param->scan_rst.bda[4], param->scan_rst.bda[5],
                        param->scan_rst.rssi);
              }
            }

            if (memcmp(&param->scan_rst.ble_adv[i], easySetupAdv, sizeof(easySetupAdv)) == 0) {
              found = true;
              sprintf(out, "FOUND EASYSETUP PACKET: %02x:%02x:%02x:%02x:%02x:%02x\t%i db",
                      param->scan_rst.bda[0], param->scan_rst.bda[1],
                      param->scan_rst.bda[2], param->scan_rst.bda[3],
                      param->scan_rst.bda[4], param->scan_rst.bda[5],
                      param->scan_rst.rssi);
            }

            if (memcmp(&param->scan_rst.ble_adv[i], loveSpouseAdv, sizeof(loveSpouseAdv)) == 0) {
              found = true;
              sprintf(out, "FOUND LOVESPOUSE PACKET: %02x:%02x:%02x:%02x:%02x:%02x\t%i db",
                      param->scan_rst.bda[0], param->scan_rst.bda[1],
                      param->scan_rst.bda[2], param->scan_rst.bda[3],
                      param->scan_rst.bda[4], param->scan_rst.bda[5],
                      param->scan_rst.rssi);
            }

            if (memcmp(&param->scan_rst.ble_adv[i], swiftPairAdv, sizeof(swiftPairAdv)) == 0) {
              found = true;
              sprintf(out, "FOUND SWIFTPAIR PACKET: %02x:%02x:%02x:%02x:%02x:%02x\t%i db",
                      param->scan_rst.bda[0], param->scan_rst.bda[1],
                      param->scan_rst.bda[2], param->scan_rst.bda[3],
                      param->scan_rst.bda[4], param->scan_rst.bda[5],
                      param->scan_rst.rssi);
            }

          }
          
          if (found) {
            ESP_LOGI(TAG, "%s", out);
            BLESpamDetectApp::sendOutput(out);

            if (ble_spam_detect_last_led == 0) {
              leds->set_color(1, 0, 0, 255);
              leds->set_color(0, 0, 0, 0);
              ble_spam_detect_last_led = 1;
            } else {
              leds->set_color(0, 0, 0, 255);
              leds->set_color(1, 0, 0, 0);
              ble_spam_detect_last_led = 0;
            }

            vTaskDelay(60 / portTICK_PERIOD_MS);
          }
          break;
        }
        default:
          break;
      }
      
      break;
    }
    case ESP_GAP_BLE_SCAN_TIMEOUT_EVT: {
      ESP_LOGI(TAG, "SCAN TIMEOUT");
      break;
    }
    case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT: {
      ESP_LOGI(TAG, "SCAN COMPLETE");
      break;
    }
    default:
      break;
  }
}

void BLESpamDetectApp::stop_detect() {
  if (bleSpamDetectTaskHandle != NULL) { 
    vTaskDelete(bleSpamDetectTaskHandle);
    bleSpamDetectTaskHandle = NULL;
  }

  BLEDevice::deinit();

  leds->blink(255, 0, 0, 2);
  bleSpamDetectState = BLE_SPAM_DETECT_STATE_HOME;
  BLESpamDetectApp::sendState((int)bleSpamDetectState);
};

void BLESpamDetectApp::start_detect() {
  esp_err_t ret = xTaskCreate(&BLESpamDetectApp::_task, "blespamd", 16000, NULL, 1, &bleSpamDetectTaskHandle);
  if (ret == ESP_FAIL) {
    ESP_LOGE(TAG, "blespamd task err: %s", esp_err_to_name(ret));
  }
  bleSpamDetectState = BLE_SPAM_DETECT_STATE_RUNNING;
  BLESpamDetectApp::sendState((int)bleSpamDetectState);
};

void BLESpamDetectApp::_task(void *taskParams) {
  ESP_LOGI(TAG, "Running");

  BLEDevice::init("");

  esp_err_t ret = esp_ble_gap_register_callback(BLESpamDetectApp::gap_cb);
  if (ret) {
    ESP_LOGE(TAG, "%s gap register failed, error code = %x", __func__, ret);
    return;
  }

  esp_ble_gap_set_scan_params(&ble_detect_scan_params);

  while (1) {
    
    vTaskDelay(20 / portTICK_PERIOD_MS);
  }
};

void BLESpamDetectApp::webEvent(BLESpamDetectWebEventParams *ps) {
  ESP_LOGI(TAG, "Web: %s", ps->action);
  JsonDocument doc;
  String res;

  if (strcmp(ps->action, "status") == 0) {
    doc["app"] = APP_ID_BLE_SPAM_DETECT;
    doc["state"] = (int)bleSpamDetectState;
    serializeJson(doc, res);
    return WebServerModule::notifyClients(res);
  }

  if (strcmp(ps->action, "start") == 0) {
    DisplayManager::appRunning = true;
    leds->stop_animations();
    bleApp = BLE_APP_SPAM_DETECT;
    DisplayManager::setMenu(BLE_MENU);
    BLESpamDetectApp::start_detect();
  }

  if (strcmp(ps->action, "stop") == 0) {
    BLESpamDetectApp::stop_detect();
    DisplayManager::appRunning = false;
    bleApp = BLE_APP_NONE;
    DisplayManager::setMenu(MAIN_MENU);
    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }
};

void BLESpamDetectApp::ontouch(int pad, const char* type, uint32_t value) {
  switch (pad) {
    case LEFT: {
      break;
    }
    case RIGHT: {
      break;
    }
    case MIDDLE: {
      if (strcmp(type, "longPress") == 0) {
        switch (bleSpamDetectState) {
          case BLE_SPAM_DETECT_STATE_HOME: {
            bleApp = BLE_APP_NONE;
            DisplayManager::appRunning = false;
            return;
          }
          case BLE_SPAM_DETECT_STATE_RUNNING: {
            stop_detect();
            return;
          }
          default:
            break;
        }
      }

      switch (bleSpamDetectState) {
        case BLE_SPAM_DETECT_STATE_HOME: {
          start_detect();
          break;
        }
        default:
          break;
      }
      
      break;
    }
  }
};

void BLESpamDetectApp::render() {
  switch (bleSpamDetectState) {
    case BLE_SPAM_DETECT_STATE_HOME: {
      display->centeredText("ble spam detect", 0);
      display->text("", 1);
      display->centeredText("start", 2);
      display->text("", 3);
      break;
    }
    case BLE_SPAM_DETECT_STATE_RUNNING: {
      display->centeredText("ble spam detect", 0);
      display->text("", 1);
      display->centeredText("running", 2);
      display->centeredText("", 3);
      break;
    }
    default:
      break;
  }

  vTaskDelay(20 / portTICK_PERIOD_MS);
};