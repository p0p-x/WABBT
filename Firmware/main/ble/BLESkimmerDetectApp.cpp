#include "BLEDevice.h"

#include "ble/BLESkimmerDetectApp.h"
#include "DisplayManager.h"

#define TAG "BLESkimmerDetect"

int ble_skimmer_detect_last_led = 0;
TaskHandle_t bleSkimmerDetectTaskHandle = NULL;
BLESkimmerDetectAppState bleSkimmerDetectState = BLE_SKIMMER_DETECT_STATE_HOME;

/**
 * sources:
 *  https://learn.sparkfun.com/tutorials/gas-pump-skimmers
 *  https://github.com/justcallmekoko/ESP32Marauder/blob/c0554b95ceb379d29b9a8925d27cc2c0377764a9/esp32_marauder/WiFiScan.cpp#L253
 */

static const char* skimmers[] = {
  "HC-03",
  "HC-05",
  "HC-06",
};

// 00001101-0000-1000-8000-00805F9B34FB
esp_bt_uuid_t bad_uuid = {
  .len = ESP_UUID_LEN_16,
  .uuid = {
    .uuid16 = 0x1101,
  },
};

esp_ble_scan_params_t ble_skimmer_detect_scan_params = {
  .scan_type = BLE_SCAN_TYPE_PASSIVE,
  .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
  .scan_filter_policy = BLE_SCAN_FILTER_ALLOW_ALL,
  .scan_interval = 0x50,
  .scan_window = 0x30,
  .scan_duplicate = BLE_SCAN_DUPLICATE_DISABLE
};

void BLESkimmerDetectApp::sendOutput(String output) {
  JsonDocument doc;
  String res;

  doc["app"] = APP_ID_BLE_SKIMMER_DETECT;
  doc["output"] = output;
  serializeJson(doc, res);
  WebServerModule::notifyClients(res);
};

void BLESkimmerDetectApp::sendState(int state) {
  JsonDocument doc;
  String res;

  doc["app"] = APP_ID_BLE_SKIMMER_DETECT;
  doc["state"] = state;
  serializeJson(doc, res);
  WebServerModule::notifyClients(res);
};

void BLESkimmerDetectApp::gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
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

          char adv_name_str[adv_name_len + 1];
          memcpy(adv_name_str, adv_name, adv_name_len);
          adv_name_str[adv_name_len] = '\0';

          for (size_t i = 0; i < sizeof(skimmers) / sizeof(skimmers[0]); ++i) {
            if (strcmp(adv_name_str, skimmers[i]) == 0) {
              char out[100];
              sprintf(out, "Potential Skimmer: %s %02x:%02x:%02x:%02x:%02x:%02x (%i)",
                      adv_name_str,
                      param->scan_rst.bda[0], param->scan_rst.bda[1], param->scan_rst.bda[2],
                      param->scan_rst.bda[3], param->scan_rst.bda[4], param->scan_rst.bda[5],
                      param->scan_rst.rssi);
              ESP_LOGI(TAG, "%s", out);
              BLESkimmerDetectApp::sendOutput(out);
              leds->blink(255, 0, 0, 2);
            }
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

void BLESkimmerDetectApp::stop_detect() {
  if (bleSkimmerDetectTaskHandle != NULL) { 
    vTaskDelete(bleSkimmerDetectTaskHandle);
    bleSkimmerDetectTaskHandle = NULL;
  }

  BLEDevice::deinit();

  leds->blink(255, 0, 0, 2);
  bleSkimmerDetectState = BLE_SKIMMER_DETECT_STATE_HOME;
  BLESkimmerDetectApp::sendState((int)bleSkimmerDetectState);
};

void BLESkimmerDetectApp::start_detect() {
  esp_err_t ret = xTaskCreate(&BLESkimmerDetectApp::_task, "bleskimd", 16000, NULL, 1, &bleSkimmerDetectTaskHandle);
  if (ret == ESP_FAIL) {
    ESP_LOGE(TAG, "bleskimd task err: %s", esp_err_to_name(ret));
  }
  bleSkimmerDetectState = BLE_SKIMMER_DETECT_STATE_RUNNING;
  BLESkimmerDetectApp::sendState((int)bleSkimmerDetectState);
};

void BLESkimmerDetectApp::_task(void *taskParams) {
  ESP_LOGI(TAG, "Running");

  BLEDevice::init("");

  esp_err_t ret = esp_ble_gap_register_callback(BLESkimmerDetectApp::gap_cb);
  if (ret) {
    ESP_LOGE(TAG, "%s gap register failed, error code = %x", __func__, ret);
    return;
  }

  esp_ble_gap_set_scan_params(&ble_skimmer_detect_scan_params);

  while (1) {
    
    vTaskDelay(20 / portTICK_PERIOD_MS);
  }
};

void BLESkimmerDetectApp::webEvent(BLESkimmerDetectWebEventParams *ps) {
  ESP_LOGI(TAG, "Web: %s", ps->action);
  JsonDocument doc;
  String res;

  if (strcmp(ps->action, "status") == 0) {
    doc["app"] = APP_ID_BLE_SKIMMER_DETECT;
    doc["state"] = (int)bleSkimmerDetectState;
    serializeJson(doc, res);
    return WebServerModule::notifyClients(res);
  }

  if (strcmp(ps->action, "start") == 0) {
    DisplayManager::appRunning = true;
    leds->stop_animations();
    bleApp = BLE_APP_SKIMMER_DETECT;
    DisplayManager::setMenu(BLE_MENU);
    BLESkimmerDetectApp::start_detect();
  }

  if (strcmp(ps->action, "stop") == 0) {
    BLESkimmerDetectApp::stop_detect();
    DisplayManager::appRunning = false;
    bleApp = BLE_APP_NONE;
    DisplayManager::setMenu(MAIN_MENU);
    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }
};

void BLESkimmerDetectApp::ontouch(int pad, const char* type, uint32_t value) {
  switch (pad) {
    case LEFT: {
      break;
    }
    case RIGHT: {
      break;
    }
    case MIDDLE: {
      if (strcmp(type, "longPress") == 0) {
        switch (bleSkimmerDetectState) {
          case BLE_SKIMMER_DETECT_STATE_HOME: {
            bleApp = BLE_APP_NONE;
            DisplayManager::appRunning = false;
            return;
          }
          case BLE_SKIMMER_DETECT_STATE_RUNNING: {
            stop_detect();
            return;
          }
          default:
            break;
        }
      }

      switch (bleSkimmerDetectState) {
        case BLE_SKIMMER_DETECT_STATE_HOME: {
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

void BLESkimmerDetectApp::render() {
  switch (bleSkimmerDetectState) {
    case BLE_SKIMMER_DETECT_STATE_HOME: {
      display->centeredText("skimmer detect", 0);
      display->text("", 1);
      display->centeredText("start", 2);
      display->text("", 3);
      break;
    }
    case BLE_SKIMMER_DETECT_STATE_RUNNING: {
      display->centeredText("skimmer detect", 0);
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