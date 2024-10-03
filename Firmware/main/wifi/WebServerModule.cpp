#include <LittleFS.h>
#include <DNSServer.h>
#include <ArduinoJson.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_system.h"

#include "settings.h"
#include "wifi/PMKIDApp.h"
#include "wifi/DeauthApp.h"
#include "wifi/DeauthDetectApp.h"
#include "wifi/WebServerModule.h"
#include "ble/BLESpamApp.h"
#include "ble/BLEDroneApp.h"
#include "ble/GattAttackApp.h"
#include "ble/BLESmartTagApp.h"
#include "ble/BLESpamDetectApp.h"
#include "ble/BLEFastPairSpamApp.h"
#include "ble/BLEEasySetupSpamApp.h"
#include "ble/BLESkimmerDetectApp.h"
#include "ble/BLEvHCIApp.h"
#include "ble/BLEReplayApp.h"

#define TAG "webserver"

DNSServer dnsServer;
AsyncWebSocket WebServerModule::ws("/ws");

AllClientsDisconnectedCallback g_disconnected_callback = NULL;

int WebServerModule::client_count = 0;
void WebServerModule::sta_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED) {
    WebServerModule::client_count++;
    ESP_LOGI(TAG, "Station connected, total connected: %d", client_count);
  } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED) {
    WebServerModule::client_count--;
    ESP_LOGI(TAG, "Station disconnected, total connected: %d", client_count);
    if (WebServerModule::client_count == 0) {
      ESP_LOGI(TAG, "All clients disconnected.");
      if (g_disconnected_callback != NULL) {
        g_disconnected_callback();
      }
    }
  }
};

WebServerModule::WebServerModule(const char* ssid, const char* password) 
    : ssid(ssid), password(password), server(80) {}

String WebServerModule::getContentType(String filename) {
  if (filename.endsWith(".htm") || filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".png")) return "image/png";
  else if (filename.endsWith(".jpg") || filename.endsWith(".jpeg")) return "image/jpeg";
  else if (filename.endsWith(".gif")) return "image/gif";
  else if (filename.endsWith(".svg")) return "image/svg+xml";
  return "text/plain";
};

void WebServerModule::notifyClients(String rsp) {
  WebServerModule::ws.textAll(rsp);
};

void WebServerModule::handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  String output;
  JsonDocument docOut;

  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    JsonDocument docIn;
    deserializeJson(docIn, data);

    BADGE_APPS opt = (BADGE_APPS)docIn["opt"];
    docOut["success"] = false;

    switch (opt) {
      case APP_ID_WIFI_DEAUTH: {
        // WiFi Deauth
        DeauthWebEventParams ps;
        
        const char* action = docIn["action"];
        int ap_id = docIn["ap_id"];
        int sta_id = docIn["sta_id"];
        uint8_t reason = docIn["reason"];

        if (action == NULL) {
          ESP_LOGI(TAG, "Deauth: Action Null");
          docOut["error"] = "action null";
          break;
        }

        ps.action = action;
        ps.ap_id = ap_id;
        ps.sta_id = sta_id;
        ps.reason = reason;
        
        return DeauthApp::webEvent(&ps);
      }
      case APP_ID_BLE_NAMESPAM: {
        // BLE Namespam
        NameSpamWebEventParams ps;
        
        const char* action = docIn["action"];
        const char* name = docIn["name"];

        if (action == NULL) {
          ESP_LOGI(TAG, "NameSpam: Action Null");
          docOut["error"] = "action null";
          break;
        }

        ps.action = action;
        ps.name = name;
        
        return BLENameSpamApp::webEvent(&ps);
      }
      case APP_ID_BLE_GATTATTACK: {
        // GATT Attack
        GattAttackWebEventParams ps;
        
        const char* action = docIn["action"];
        int id = docIn["id"];

        if (action == NULL) {
          ESP_LOGI(TAG, "GattAttack: Action Null");
          docOut["error"] = "action null";
          break;
        }

        ps.action = action;
        ps.id = id;
        
        return GattAttackApp::webEvent(&ps);
      }
      case APP_ID_SETTINGS_NAME: {
        // Settings - Name
        const char* name = docIn["name"];
        ESP_LOGI(TAG, "Websocket: setName %s", name);
        
        if (name != NULL) {
          if (name == nullptr || strlen(name) == 0) {
            docOut["success"] = false;
            docOut["error"] = "You at least want to be called something..";
            break;
          }

          if (strncpy(badge_name, name, 16) != 0) {
            esp_err_t ret = write_string_to_nvs("badge_name", name);
            if (ret != ESP_OK) {
              docOut["success"] = false;
              docOut["error"] = esp_err_to_name(ret);
            } else {
              docOut["success"] = true;
            }
            break;
          } else {
            docOut["error"] = "Bad shit";
          }
          
        } else {
          docOut["success"] = false;
          docOut["error"] = "Invalid Name";
          break;
        }

        docOut["success"] = false;

        break;
      }
      case APP_ID_SETTINGS_WIFI: {
        // Settings - wifi ssid/pass
        const char* ssid = docIn["ssid"];
        const char* pass = docIn["pass"];
        ESP_LOGI(TAG, "Websocket: set_ssid %s", ssid);

        if (ssid != NULL && pass != NULL) {
          if (ssid == nullptr || strlen(ssid) == 0) {
            docOut["success"] = false;
            docOut["error"] = "SSID cannot be blank";
            break;
          }

          if (pass == nullptr || strlen(pass) < 8) {
            docOut["success"] = false;
            docOut["error"] = "PASS cannot be blank and must be longer than 8 characters.";
            break;
          }
          
          if (strncpy(WIFI_SSID, ssid, 32) != 0 && strncpy(WIFI_PASS, pass, 32) != 0) {
            esp_err_t ret = write_string_to_nvs("wifi_ssid", ssid);
            ret = write_string_to_nvs("wifi_pass", pass);
            if (ret != ESP_OK) {
              docOut["success"] = false;
              docOut["error"] = esp_err_to_name(ret);
            } else {
              docOut["success"] = true;
            }
            break;
          } else {
            docOut["error"] = "Failed to set wifi";
          }
          
        } else {
          docOut["success"] = false;
          docOut["error"] = "Invalid Params";
          break;
        }

        docOut["success"] = false;

        break;
      }
      case APP_ID_WIFI_DEAUTH_DETECT: {
        // Deauth Detect
        DeauthDetectWebEventParams ps;

        const char* action = docIn["action"];

        if (action == NULL) {
          ESP_LOGI(TAG, "DeauthDetect: Action Null");
          docOut["error"] = "action null";
          break;
        }

        ps.action = action;

        return DeauthDetectApp::webEvent(&ps);
      }
      case APP_ID_BLE_FASTPAIR_SPAM: {
        // FastPair Spam
        BLEFastPairSpamWebEventParams ps;

        const char* action = docIn["action"];

        if (action == NULL) {
          ESP_LOGI(TAG, "FastPair: Action Null");
          docOut["error"] = "action null";
          break;
        }

        ps.action = action;

        return BLEFastPairSpamApp::webEvent(&ps);
      }
      case APP_ID_BLE_SPAM_DETECT: {
        // BLE Spam Detect
        BLESpamDetectWebEventParams ps;

        const char* action = docIn["action"];

        if (action == NULL) {
          ESP_LOGI(TAG, "BLESpamDetect: Action Null");
          docOut["error"] = "action null";
          break;
        }

        ps.action = action;

        return BLESpamDetectApp::webEvent(&ps);
      }
      case APP_ID_BLE_VHCI: {
        // BLE virtual HCI
        BLEvHCIWebEventParams ps;

        const char* action = docIn["action"];

        if (action == NULL) {
          ESP_LOGI(TAG, "BLEvHCI: Action Null");
          docOut["error"] = "action null";
          break;
        }

        ps.action = action;
        ps.hex_buffer = docIn["input"];

        return BLEvHCIApp::webEvent(&ps);
      }
      case APP_ID_BLE_EASYSETUP_SPAM: {
        // BLE EasySetup Spam
        BLEEasySetupSpamWebEventParams ps;

        const char* action = docIn["action"];

        if (action == NULL) {
          ESP_LOGI(TAG, "EasySetupSpam: Action Null");
          docOut["error"] = "action null";
          break;
        }

        ps.action = action;

        return BLEEasySetupSpamApp::webEvent(&ps);
      }
      case APP_ID_WIFI_PMKID: {
        // WIFI PMKID
        PMKIDWebEventParams ps;

        const char* action = docIn["action"];

        if (action == NULL) {
          ESP_LOGI(TAG, "PMKID: Action Null");
          docOut["error"] = "action null";
          break;
        }

        ps.action = action;
        ps.channel = docIn["chan"];
        ps.ap_id = docIn["ap_id"];

        return PMKIDApp::webEvent(&ps);
      }
      case APP_ID_BLE_SKIMMER_DETECT: {
        // BLE Skimmer Detect
        BLESkimmerDetectWebEventParams ps;

        const char* action = docIn["action"];

        if (action == NULL) {
          ESP_LOGI(TAG, "Skimmer: Action Null");
          docOut["error"] = "action null";
          break;
        }

        ps.action = action;

        return BLESkimmerDetectApp::webEvent(&ps);
      }
      case APP_ID_BLE_DRONE: {
        // BLE Fake Drone
        BLEDroneWebEventParams ps;

        const char* action = docIn["action"];

        if (action == NULL) {
          ESP_LOGI(TAG, "Drone: Action Null");
          docOut["error"] = "action null";
          break;
        }

        ps.action = action;
        ps.name = docIn["name"];
        ps.lat = docIn["lat"];
        ps.lng = docIn["lng"];

        return BLEDroneApp::webEvent(&ps);
      }
      case APP_ID_BLE_SMART_TAG: {
        // BLE Fake Drone
        BLESmartTagWebEventParams ps;

        const char* action = docIn["action"];

        if (action == NULL) {
          ESP_LOGI(TAG, "SmartTag: Action Null");
          docOut["error"] = "action null";
          break;
        }

        ps.action = action;

        return BLESmartTagApp::webEvent(&ps);
      }
      /**
      case APP_ID_BLE_REPLAY: {
        // BLE Replay
        BLEReplayWebEventParams ps;

        const char* action = docIn["action"];

        if (action == NULL) {
          ESP_LOGI(TAG, "Replay: Action Null");
          docOut["error"] = "action null";
          break;
        }

        ps.action = action;
        ps.id = docIn["id"];
        ps.send = docIn["send"];

        return BLEReplayApp::webEvent(&ps);
      }
      */
      case APP_ID_SETTINGS_TOUCH_THRESHOLD: {
        // Settings - Touch Threshold
        int value = docIn["value"];
        ESP_LOGI(TAG, "Websocket: setTouch %i", value);
        
        if (value > 0) {
            esp_err_t ret = write_uint32_to_nvs("touch_value", value);
            if (ret != ESP_OK) {
              docOut["success"] = false;
              docOut["error"] = esp_err_to_name(ret);
            } else {
              docOut["success"] = true;
            }
            break;
        } else {
          docOut["success"] = false;
          docOut["error"] = "Invalid Value";
          break;
        }

        docOut["success"] = false;

        break;
      }
      case APP_ID_SETTINGS_RESTART: {
        // Settings - Touch Threshold
        int value = docIn["value"];
        ESP_LOGI(TAG, "Websocket: reboot %i", value);
        
        if (value > 9000) {
          esp_restart();
        } else {
          docOut["success"] = false;
          docOut["error"] = "Invalid Value";
          break;
        }

        docOut["success"] = false;

        break;
      }
      default:
        // Bad
        docOut["error"] = "Invalid opt";
        break;
    }

    serializeJson(docOut, output);
    WebServerModule::notifyClients(output);
  }
};

void WebServerModule::onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      ESP_LOGI(TAG, "WebSocket client #%" PRIu32 " connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      ESP_LOGI(TAG, "WebSocket client #%" PRIu32 " disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      WebServerModule::handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
};

void WebServerModule::startAP() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(local_IP, gateway, subnet);
  WiFi.softAP(WIFI_SSID, WIFI_PASS);
  ESP_LOGI(TAG, "WiFi Access Point started. IP Address: %s", WiFi.softAPIP().toString().c_str());
  ESP_LOGI(TAG, "sta: %s", WiFi.macAddress().c_str());
  ESP_LOGI(TAG, "ap: %s", WiFi.softAPmacAddress().c_str());
};

void WebServerModule::stopAP() {
  esp_wifi_set_promiscuous(false);
  ESP_ERROR_CHECK(esp_wifi_deauth_sta(0));
  WiFi.softAPdisconnect();
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
};

void WebServerModule::setup() {
  esp_event_loop_create_default();
  esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &WebServerModule::sta_handler, NULL);

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(local_IP, gateway, subnet);
  WiFi.softAP(ssid, password);
  ESP_LOGI(TAG, "WiFi Access Point started. IP Address: %s", WiFi.softAPIP().toString().c_str());
  ESP_LOGI(TAG, "sta: %s", WiFi.macAddress().c_str());
  ESP_LOGI(TAG, "ap: %s", WiFi.softAPmacAddress().c_str());

  if (dnsServer.start()) {
    ESP_LOGI(TAG, "DNSServer Started");
  }

  esp_wifi_get_channel(&PMKIDApp::channel, NULL);

  if (ap_mac_address[0] & 0x01) {
    ESP_LOGE(TAG, "AP Might not show due to ap_mac_address[0] & 0x01");
  }

  if (!LittleFS.begin()) {
    ESP_LOGE(TAG, "An Error has occurred while mounting LittleFS");
  }

  ws.onEvent(WebServerModule::onEvent);
  server.addHandler(&ws);

  server.onNotFound([this](AsyncWebServerRequest *request) {
    String path = request->url();
    if (path.endsWith("/")) path += "index.html";

    if (path.endsWith("204")) {
      AsyncWebServerResponse *res = request->beginResponse(302);
      res->addHeader("Location", "http://1.3.3.7");
      return request->send(res);
    }

    String gzipPath = path + ".gz";
    if (LittleFS.exists(gzipPath)) {
      AsyncWebServerResponse *res = request->beginResponse(LittleFS, gzipPath, this->getContentType(path));
      res->addHeader("Content-Encoding", "gzip");
      request->send(res);
    } else if (LittleFS.exists(path)) {
      request->send(LittleFS, path, this->getContentType(path));
    } else {
      AsyncWebServerResponse *res = request->beginResponse(302);
      res->addHeader("Location", "http://1.3.3.7");
      request->send(res);
    }
  });

  server.begin();
  ESP_LOGI(TAG, "HTTP server started");
};

void WebServerModule::loop() {}
