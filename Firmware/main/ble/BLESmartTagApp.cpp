#include "BLEDevice.h"

#include "ble/BLESmartTagApp.h"
#include "DisplayManager.h"

#define TAG "BLESmartTag"

int ble_spam_detect_last_led = 0;

TaskHandle_t bleSmartTagTaskHandle = NULL;
BLESmartTagAppState bleSmartTagState = BLE_SMART_TAG_STATE_HOME;

enum tagType {
  TAG_AIRTAG,
  TAG_GENERIC1,
  TAG_TILE,
};

struct client {
  esp_gatt_if_t interface = ESP_GATT_IF_NONE;
  esp_ble_addr_type_t addr_type;
  esp_bd_addr_t addr;
  uint16_t start_handle = 0;
  uint16_t end_handle = 0;
  tagType tag;
  bool connecting = false;
  bool connected = false;
};

client cli1;
int in_use = 0;
int num_clients = 1;

bool cli_on_target(esp_bd_addr_t target) {
  if (memcmp(cli1.addr, target, 6) == 0) {
    return true;
  }

  return false;
};

client& get_cli_by_if(esp_gatt_if_t interface) {
  if (cli1.interface == interface) {
    return cli1;
  }

  return cli1;
};

esp_bt_uuid_t generic_srv_uuid = {
  .len = ESP_UUID_LEN_16,
  .uuid = {
    .uuid16 = 0x1802
  }
};

esp_bt_uuid_t generic_char_uuid = {
  .len = ESP_UUID_LEN_16,
  .uuid = {
    .uuid16 = 0x2a06
  }
};

esp_bt_uuid_t airplay_srv_uuid = {
  .len = ESP_UUID_LEN_128,
  .uuid = {
    .uuid128 = {
      0x6C, 0xD6, 0xF8, 0x28, 0x97, 0x8D, 0xAA, 0x86,
      0x51, 0x49, 0x1C, 0x7D, 0x00, 0x90, 0xFC, 0x7D
    }
  }
};

esp_bt_uuid_t airplay_char_uuid = {
  .len = ESP_UUID_LEN_128,
  .uuid = {
    .uuid128 = {
      0x6C, 0xD6, 0xF8, 0x28, 0x97, 0x8D, 0xAA, 0x86,
      0x51, 0x49, 0x1C, 0x7D, 0x01, 0x90, 0xFC, 0x7D
    }
  }
};

const char* adv_names[] = {
  "Smart Tag",
  "smart tag",
  "Smart Tag ",
};

int num_names = (sizeof(adv_names) / sizeof(adv_names[0]));

esp_ble_scan_params_t ble_smart_tag_scan_params = {
  .scan_type = BLE_SCAN_TYPE_ACTIVE,
  .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
  .scan_filter_policy = BLE_SCAN_FILTER_ALLOW_ALL,
  .scan_interval = 0x50,
  .scan_window = 0x30,
  .scan_duplicate = BLE_SCAN_DUPLICATE_DISABLE
};

void BLESmartTagApp::sendOutput(String output) {
  ESP_LOGI(TAG, "%s", output.c_str());
  JsonDocument doc;
  String res;

  doc["app"] = APP_ID_BLE_SMART_TAG;
  doc["output"] = output;
  serializeJson(doc, res);
  WebServerModule::notifyClients(res);
};

void BLESmartTagApp::sendState(int state) {
  JsonDocument doc;
  String res;

  doc["app"] = APP_ID_BLE_SMART_TAG;
  doc["state"] = state;
  serializeJson(doc, res);
  WebServerModule::notifyClients(res);
};

void BLESmartTagApp::client_event(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param) {
  client& cli = get_cli_by_if(gattc_if);

  switch (event) {
    case ESP_GATTC_REG_EVT: {
      if (cli1.interface ==  ESP_GATT_IF_NONE) {
        cli1.interface = gattc_if;
        break;
      }      
      break;
    }
    case ESP_GATTC_CONNECT_EVT: {
      cli.connecting = false;
      cli.connected = true;
      ESP_LOGI(TAG, "Connected: conn_id %x, if %x", param->connect.conn_id, gattc_if);
      break;
    }
    case ESP_GATTC_OPEN_EVT: {
      if (param->open.status != ESP_GATT_OK) {
        ESP_LOGE(TAG, "Open Target connection failed: %d", param->open.status);
        break;
      }
      break;
    }
    case ESP_GATTC_DIS_SRVC_CMPL_EVT: {
      if (param->dis_srvc_cmpl.status != ESP_GATT_OK) {
        ESP_LOGE(TAG, "discover service failed, status %d", param->dis_srvc_cmpl.status);
        break;
      }
      esp_ble_gattc_search_service(gattc_if, param->dis_srvc_cmpl.conn_id, NULL);
      break;
    }
    case ESP_GATTC_SEARCH_RES_EVT: {
      if (param->search_res.srvc_id.uuid.len == ESP_UUID_LEN_16) { 
        //ESP_LOGI(TAG, "Service UUID16: %x", param->search_res.srvc_id.uuid.uuid.uuid16);

        if (param->search_res.srvc_id.uuid.uuid.uuid16 == generic_srv_uuid.uuid.uuid16) {
          cli.start_handle = param->search_res.start_handle;
          cli.end_handle = param->search_res.end_handle;
          cli.tag = TAG_GENERIC1;
        }
      } else if (param->search_res.srvc_id.uuid.len == ESP_UUID_LEN_32) {
        ESP_LOGI(TAG, "Service UUID32: 0x%08" PRIx32, param->search_res.srvc_id.uuid.uuid.uuid32);
      } else {
        //ESP_LOGI(TAG, "Service UUID128:");
        //esp_log_buffer_hex(TAG, param->search_res.srvc_id.uuid.uuid.uuid128, 16);

        if (memcmp(param->search_res.srvc_id.uuid.uuid.uuid128, airplay_srv_uuid.uuid.uuid128, 16) == 0) {
          cli.start_handle = param->search_res.start_handle;
          cli.end_handle = param->search_res.end_handle;
          cli.tag = TAG_AIRTAG;
        }
      }
      break;
    }
    case ESP_GATTC_SEARCH_CMPL_EVT: {
      if (param->search_cmpl.status != ESP_GATT_OK) {
        ESP_LOGE(TAG, "search service failed, error status = %x", param->search_cmpl.status);
        break;
      }

      uint16_t count = 1;
      esp_gattc_char_elem_t result;
      esp_gatt_status_t status;
      esp_bt_uuid_t uuid;

      if (cli.tag == TAG_AIRTAG) {
        uuid = airplay_char_uuid;
      } else if (cli.tag  == TAG_GENERIC1) {
        uuid = generic_char_uuid;
      }
      
      status = esp_ble_gattc_get_char_by_uuid(gattc_if,
                                              param->search_cmpl.conn_id,
                                              cli.start_handle,
                                              cli.end_handle,
                                              uuid,
                                              &result,
                                              &count);
      if (status != ESP_GATT_OK) {
        ESP_LOGE(TAG, "esp_ble_gattc_get_char_by_uuid error: %02x", status);
        break;
      }

      if (count > 0) {
        uint16_t value_len = 1;
        uint8_t value[1];

        if (cli.tag == TAG_AIRTAG) {
          value[0] = { 0xAF };
        } else if (cli.tag == TAG_GENERIC1) {
          value[0] = { 0x02 };
        }
        ESP_LOGI(TAG, "Write to handle: %" PRIX16 "  value: '0x%02x'", result.char_handle, value[0]);
        esp_err_t ret = esp_ble_gattc_write_char(gattc_if, param->search_res.conn_id,
                                result.char_handle, value_len, value,
                                ESP_GATT_WRITE_TYPE_RSP, ESP_GATT_AUTH_REQ_NONE);
      } else {
        esp_ble_gattc_close(gattc_if, param->search_res.conn_id);
      }
      
      break;
    }
    case ESP_GATTC_WRITE_CHAR_EVT: {
      if (param->write.status != ESP_GATT_OK) {
        ESP_LOGI(TAG, "WRITE ERR handle: %"PRIx16 " status: %i  %s", param->write.handle, param->write.status, esp_err_to_name(param->write.status));
      } else {
        ESP_LOGI(TAG, "WRITE SUCCESS");
      }

      esp_ble_gattc_close(gattc_if, param->write.conn_id);
      break;
    }
    case ESP_GATTC_DISCONNECT_EVT: {
      in_use--;
      cli.connecting = false;
      cli.connected = false;
      memset(cli.addr, 0, 6);
      break;
    }
    default:
      break;
  }
};

void BLESmartTagApp::gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
  uint8_t *adv_name = NULL;
  uint8_t adv_name_len = 0;
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
          if (cli_on_target(param->scan_rst.bda)) {
            break;
          }

          uint8_t* pkt = param->scan_rst.ble_adv;
          adv_name = esp_ble_resolve_adv_data(pkt,
                                              ESP_BLE_AD_TYPE_NAME_CMPL,
                                              &adv_name_len);

          char out[100];
          bool found = false;
          bool should_connect =  false;

          if (adv_name != NULL) {
            char name_str[adv_name_len + 1];
            memcpy(name_str, adv_name, adv_name_len);
            name_str[adv_name_len] = '\0';

            for (int i = 0; i < num_names; ++i) {
              if (strcmp(name_str, adv_names[i]) == 0) {
                found = true;
                sprintf(out, "Found ADV '%s': %02x:%02x:%02x:%02x:%02x:%02x  (%i)",
                      adv_names[i],
                      param->scan_rst.bda[0], param->scan_rst.bda[1], param->scan_rst.bda[2],
                      param->scan_rst.bda[3], param->scan_rst.bda[4], param->scan_rst.bda[5],
                      param->scan_rst.rssi);
              }
            }
          }

          for (int i = 0; i < param->scan_rst.adv_data_len; i++) {
            uint8_t b = pkt[i];

            // AirTag
            if ((b == 0x4C && pkt[i + 1] == 0x00 && pkt[i + 3] == 0x19)) {
              found = true;
              should_connect = true;
              sprintf(out, "Found AirTag: %02x:%02x:%02x:%02x:%02x:%02x  (%i)",
                    param->scan_rst.bda[0], param->scan_rst.bda[1], param->scan_rst.bda[2],
                    param->scan_rst.bda[3], param->scan_rst.bda[4], param->scan_rst.bda[5],
                    param->scan_rst.rssi);
            }

            // Generic Smart Tag?
            if ((b == 0x5A || b == 0x59) && pkt[i + 1] == 0xFD) {
              found = true;
              sprintf(out, "Found Generic Smart Tag: %02x:%02x:%02x:%02x:%02x:%02x  (%i)",
                    param->scan_rst.bda[0], param->scan_rst.bda[1], param->scan_rst.bda[2],
                    param->scan_rst.bda[3], param->scan_rst.bda[4], param->scan_rst.bda[5],
                    param->scan_rst.rssi);
            }

            // Tile
            if ((b == 0xED || b == 0xEC) && pkt[i + 1] == 0xFE) {
              found = true;
              sprintf(out, "Found Tile: %02x:%02x:%02x:%02x:%02x:%02x  (%i)",
                    param->scan_rst.bda[0], param->scan_rst.bda[1], param->scan_rst.bda[2],
                    param->scan_rst.bda[3], param->scan_rst.bda[4], param->scan_rst.bda[5],
                    param->scan_rst.rssi);
            }

            // Chipolo
            if ((b == 0x33 || b == 0x65) && pkt[i + 1] == 0xFE) {
              found = true;
              sprintf(out, "Found Chipolo: %02x:%02x:%02x:%02x:%02x:%02x  (%i)",
                    param->scan_rst.bda[0], param->scan_rst.bda[1], param->scan_rst.bda[2],
                    param->scan_rst.bda[3], param->scan_rst.bda[4], param->scan_rst.bda[5],
                    param->scan_rst.rssi);
            }
          }

          if (found) {
            BLESmartTagApp::sendOutput(out);

            if (ble_spam_detect_last_led == 0) {
              leds->set_color(1, 0, 0, 255);
              leds->set_color(0, 0, 0, 0);
              ble_spam_detect_last_led = 1;
            } else {
              leds->set_color(0, 0, 0, 255);
              leds->set_color(1, 0, 0, 0);
              ble_spam_detect_last_led = 0;
            }

            if (should_connect) {
              client& cli = cli1;
              
              if (cli.connecting || cli.connected) {
                break;
              }

              cli.connecting = true;
              cli.addr_type = param->scan_rst.ble_addr_type;
              memcpy(cli.addr, param->scan_rst.bda, 6);
              esp_ble_gattc_open(cli.interface, param->scan_rst.bda, cli.addr_type, true);
              in_use++;
            }
          }
          break;
        }
        default:
          break;
      }
      
      break;
    }
    case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT: {
      ESP_LOGI(TAG, "Update connection params status: %d, min_int: %d, max_int: %d, latency: %d, timeout: %d",
        param->update_conn_params.status,
        param->update_conn_params.min_int,
        param->update_conn_params.max_int,
        param->update_conn_params.latency,
        param->update_conn_params.timeout);
      break;
    }
    default:
      break;
  }
}

void BLESmartTagApp::stop() {
  if (bleSmartTagTaskHandle != NULL) { 
    vTaskDelete(bleSmartTagTaskHandle);
    bleSmartTagTaskHandle = NULL;
  }

  BLEDevice::deinit();

  leds->blink(255, 0, 0, 2);
  bleSmartTagState = BLE_SMART_TAG_STATE_HOME;
  BLESmartTagApp::sendState((int)bleSmartTagState);
};

void BLESmartTagApp::start() {
  esp_err_t ret = xTaskCreate(&BLESmartTagApp::_task, "smarttag", 16000, NULL, 1, &bleSmartTagTaskHandle);
  if (ret == ESP_FAIL) {
    ESP_LOGE(TAG, "smarttag task err: %s", esp_err_to_name(ret));
  }
  bleSmartTagState = BLE_SMART_TAG_STATE_RUNNING;
  BLESmartTagApp::sendState((int)bleSmartTagState);
};

void BLESmartTagApp::_task(void *taskParams) {
  ESP_LOGI(TAG, "Running");

  BLEDevice::init("");

  esp_err_t ret = esp_ble_gap_register_callback(BLESmartTagApp::gap_cb);
  if (ret) {
    ESP_LOGE(TAG, "%s gap register failed, error code = %x", __func__, ret);
    return;
  }

  esp_ble_gap_set_scan_params(&ble_smart_tag_scan_params);

  ret = esp_ble_gattc_register_callback(BLESmartTagApp::client_event);
  if (ret) {
    ESP_LOGE(TAG, "%s gattc register failed, error code = %x", __func__, ret);
    return;
  }

  ret = esp_ble_gattc_app_register(0);
  if (ret) {
    ESP_LOGE(TAG, "%s gattc app register failed, error code = %x", __func__, ret);
  }

  while (1) {}
};

void BLESmartTagApp::webEvent(BLESmartTagWebEventParams *ps) {
  ESP_LOGI(TAG, "Web: %s", ps->action);
  JsonDocument doc;
  String res;

  if (strcmp(ps->action, "status") == 0) {
    doc["app"] = APP_ID_BLE_SMART_TAG;
    doc["state"] = (int)bleSmartTagState;
    serializeJson(doc, res);
    return WebServerModule::notifyClients(res);
  }

  if (strcmp(ps->action, "start") == 0) {
    DisplayManager::appRunning = true;
    leds->stop_animations();
    bleApp = BLE_APP_SMART_TAG;
    DisplayManager::setMenu(BLE_MENU);
    BLESmartTagApp::start();
  }

  if (strcmp(ps->action, "stop") == 0) {
    BLESmartTagApp::stop();
    DisplayManager::appRunning = false;
    bleApp = BLE_APP_NONE;
    DisplayManager::setMenu(MAIN_MENU);
    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }
};

void BLESmartTagApp::ontouch(int pad, const char* type, uint32_t value) {
  switch (pad) {
    case LEFT: {
      break;
    }
    case RIGHT: {
      break;
    }
    case MIDDLE: {
      if (strcmp(type, "longPress") == 0) {
        switch (bleSmartTagState) {
          case BLE_SMART_TAG_STATE_HOME: {
            bleApp = BLE_APP_NONE;
            DisplayManager::appRunning = false;
            return;
          }
          case BLE_SMART_TAG_STATE_RUNNING: {
            stop();
            return;
          }
          default:
            break;
        }
      }

      switch (bleSmartTagState) {
        case BLE_SMART_TAG_STATE_HOME: {
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

void BLESmartTagApp::render() {
  switch (bleSmartTagState) {
    case BLE_SMART_TAG_STATE_HOME: {
      display->centeredText("smart tagger", 0);
      display->text("", 1);
      display->centeredText("start", 2);
      display->text("", 3);
      break;
    }
    case BLE_SMART_TAG_STATE_RUNNING: {
      display->centeredText("smart tagger", 0);
      display->text("", 1);
      display->centeredText("running", 2);
      display->text("", 3);
      break;
    }
    default:
      break;
  }

  vTaskDelay(20 / portTICK_PERIOD_MS);
};