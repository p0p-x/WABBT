#include "wifi/DeauthDetectApp.h"
#include "DisplayManager.h"

#define TAG "DeauthDetect"

extern WIFIApp wifiApp;

int packetFound = 0;
int last_led = 0;
TaskHandle_t deauthDetectTaskHandle = NULL;
DeauthDetectState deauthDetectState = WIFI_DEAUTH_DETECT_STATE_HOME;

/**
 * @brief Decomplied function that overrides original one at compilation time.
 * Needs target_link_libraries(${COMPONENT_LIB} -Wl,-zmuldefs) in CMakeLists.txt
 * 
 * @attention This function is not meant to be called!
 * @see Project with original idea/implementation https://github.com/GANESH-ICMC/esp32-deauther
 */
extern "C" int ieee80211_raw_frame_sanity_check(int32_t arg, int32_t arg2, int32_t arg3){
  return 0;
}

const char* format_mac(const uint8_t mac[6]) {
  // Example MAC string: "01:23:45:67:89:AB\0"
  char* mac_str = (char *)malloc(18);
  if (mac_str == NULL) {
    return NULL;
  }

  sprintf(mac_str, "%02X:%02X:%02X:%02X:%02X:%02X",
          mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  return mac_str;
}

// tell websocket clients to ask for new results
void DeauthDetectApp::sendOutput(String msg) {
  JsonDocument doc;
  String res;

  doc["app"] = APP_ID_WIFI_DEAUTH_DETECT;
  doc["output"] = msg;
  serializeJson(doc, res);
  WebServerModule::notifyClients(res);
};

void DeauthDetectApp::sendState(int state) {
  JsonDocument doc;
  String res;

  doc["app"] = APP_ID_WIFI_DEAUTH_DETECT;
  doc["state"] = state;
  serializeJson(doc, res);
  WebServerModule::notifyClients(res);
};

void DeauthDetectApp::on_rx_packet(void *buf, wifi_promiscuous_pkt_type_t type) {
  wifi_promiscuous_pkt_t *packet = (wifi_promiscuous_pkt_t *)buf;

  switch (type) {
    case WIFI_PKT_MGMT: {
      uint8_t* macTo = &packet->payload[4];
      uint8_t* macFrom = &packet->payload[10];
      int rssi = packet->rx_ctrl.rssi;

      // TODO: find the deauth packets :)
      //ESP_LOGI(TAG, "First packet byte: %x", packet->payload[0]);
      if (packet->payload[0] == 0xc0 || packet->payload[0] == 0xa0) {
        uint8_t reason = packet->payload[24];
        char msg[100];
        sprintf(msg, "Found %s Packet: %s -> %s %idb reason: %x",
                packet->payload[0] == 0xc0 ? "Deauth" : "Disassociate",
                format_mac(macFrom),
                format_mac(macTo),
                rssi,
                reason);
        ESP_LOGI(TAG, "%s", msg);
        leds->set_color(last_led, 0, 0, 255);
        if (last_led == 0) {
          leds->set_color(1, 0, 0, 255);
          leds->set_color(0, 0, 0, 0);
          last_led = 1;
        } else {
          leds->set_color(0, 0, 0, 255);
          leds->set_color(1, 0, 0, 0);
          last_led = 0;
        }
        DeauthDetectApp::sendOutput(msg);
        packetFound++;
        vTaskDelay(100 / portTICK_PERIOD_MS);
      }

      break;
    }
    default:
      break;
  }
};

void DeauthDetectApp::stop_detect() {
  deauthDetectState = WIFI_DEAUTH_DETECT_STATE_HOME;
  DeauthDetectApp::sendState((int)deauthDetectState);

  leds->blink(255, 0, 0, 2);

  if (deauthDetectTaskHandle != NULL) { 
    vTaskDelete(deauthDetectTaskHandle);
    deauthDetectTaskHandle = NULL;
  }
  
  esp_wifi_set_promiscuous(false);
  packetFound = 0;
};

void DeauthDetectApp::start_detect() {
  esp_err_t ret = xTaskCreate(&DeauthDetectApp::_task, "deauthdetect", 16000, NULL, 1, &deauthDetectTaskHandle);
  if (ret == ESP_FAIL) {
    ESP_LOGE(TAG, "deauthdetect task err: %s", esp_err_to_name(ret));
  }
  deauthDetectState = WIFI_DEAUTH_DETECT_STATE_RUNNING;
  DeauthDetectApp::sendState((int)deauthDetectState);
};

void DeauthDetectApp::_task(void *taskParams) {
  esp_wifi_set_promiscuous_rx_cb(DeauthDetectApp::on_rx_packet);
  esp_wifi_set_promiscuous(true);

  leds->blink(0, 255, 0);

  while (1) {
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
};

void DeauthDetectApp::webEvent(DeauthDetectWebEventParams *ps) {
  ESP_LOGI(TAG, "Web: %s", ps->action);
  JsonDocument doc;
  String res;

  if (strcmp(ps->action, "status") == 0) {
    doc["app"] = APP_ID_WIFI_DEAUTH_DETECT;
    doc["state"] = (int)deauthDetectState;
    serializeJson(doc, res);
    return WebServerModule::notifyClients(res);
  }

  if (strcmp(ps->action, "start") == 0) {
    DisplayManager::appRunning = true;
    leds->stop_animations();
    wifiApp = WIFI_APP_DEAUTH_DETECT;
    DisplayManager::setMenu(WIFI_MENU);
    DeauthDetectApp::start_detect();
  }

  if (strcmp(ps->action, "stop") == 0) {
    DeauthDetectApp::stop_detect();
    DisplayManager::appRunning = false;
    wifiApp = WIFI_APP_NONE;
    DisplayManager::setMenu(MAIN_MENU);
    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }
};

void DeauthDetectApp::ontouch(int pad, const char* type, uint32_t value) {
  switch (pad) {
    case LEFT: {
      break;
    }
    case RIGHT: {
      break;
    }
    case MIDDLE: {
      if (strcmp(type, "longPress") == 0) {
        switch (deauthDetectState) {
          case WIFI_DEAUTH_DETECT_STATE_HOME: {
            wifiApp = WIFI_APP_NONE;
            DisplayManager::appRunning = false;
            return;
          }
          case WIFI_DEAUTH_DETECT_STATE_RUNNING: {
            stop_detect();
            return;
          }
          default:
            break;
        }
      }

      switch (deauthDetectState) {
        case WIFI_DEAUTH_DETECT_STATE_HOME: {
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

void DeauthDetectApp::render() {
  switch (deauthDetectState) {
    case WIFI_DEAUTH_DETECT_STATE_HOME: {
      display->centeredText("wifi/deauth_detect", 0);
      display->text("", 1);
      display->centeredText("start", 2);
      display->text("", 3);
      break;
    }
    case WIFI_DEAUTH_DETECT_STATE_RUNNING: {
      display->centeredText("main/wifi/deauth_detect", 0);
      display->text("", 1);
      display->centeredText("running", 2);
      char out[19];
      sprintf(out, "pkts: %i", packetFound);
      display->centeredText(out, 3);
      break;
    }
    default:
      break;
  }

  vTaskDelay(20 / portTICK_PERIOD_MS);
};