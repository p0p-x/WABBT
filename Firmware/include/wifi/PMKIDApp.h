#ifndef PMKIDAPP_H
#define PMKIDAPP_H

#include <ArduinoJson.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_event.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_err.h"

#include "settings.h"
#include "Display.h"
#include "LEDStrip.h"
#include "wifi/WIFIMenu.h"
#include "wifi/WebServerModule.h"

extern LEDStrip* leds;
extern Display* display;

// structs from https://github.com/FroggMaster/ESP32-Wi-Fi-Penetration-Tool/blob/flipper/components/frame_analyzer/interface/frame_analyzer_types.h

typedef struct {
    uint8_t protocol_version:2;
    uint8_t type:2;
    uint8_t subtype:4;
    uint8_t to_ds:1;
    uint8_t from_ds:1;
    uint8_t more_fragments:1;
    uint8_t retry:1;
    uint8_t power_management:1;
    uint8_t more_data:1;
    uint8_t protected_frame:1;
    uint8_t htc_order:1;
} frame_control_t;

typedef struct {
    frame_control_t frame_control;
    uint16_t duration;
    uint8_t addr1[6];
    uint8_t addr2[6];
    uint8_t addr3[6];
    uint16_t sequence_control;
} data_frame_mac_header_t;

typedef struct {
    data_frame_mac_header_t mac_header;
    uint8_t body[];
} data_frame_t;

/**
 * Size: 4 bytes
 * @see Ref: 802.1X-2020 [11.3]
 */
typedef struct {
	uint8_t version;
	uint8_t packet_type;
	uint16_t packet_body_length;
} eapol_packet_header_t;

/**
 * @see Ref: 802.1X-2020 [11.3], 802.11-2016 [12.7.2]
 */
typedef struct {
	eapol_packet_header_t header;
	uint8_t packet_body[];
} eapol_packet_t;

/**
 * Size: 2 bytes
 * @note unnamed fields are "reserved"
 * @see Ref: 802.11-2016 [12.7.2]
 */
typedef struct {
    uint8_t key_descriptor_version:3;
    uint8_t key_type:1;
    uint8_t :2;
    uint8_t install:1;
    uint8_t key_ack:1;
    uint8_t key_mic:1;
    uint8_t secure:1;
    uint8_t error:1;
    uint8_t request:1;
    uint8_t encrypted_key_data:1;
    uint8_t smk_message:1;
    uint8_t :2;
} key_information_t;

/**
 * @todo MIC lenght is dependent on 802.11-2016 [12.7.3] - key_infromation_t.descrptor_version
 * @see Ref: 802.11-2016 [12.7.2]
 */
typedef struct __attribute__((__packed__)) {
    uint8_t descriptor_type;
    key_information_t key_information;
    uint16_t key_length;
    uint8_t key_replay_counter[8];
    uint8_t key_nonce[32];
    uint8_t key_iv[16];
    uint8_t key_rsc[8];
    uint8_t reserved[8];
    uint8_t key_mic[16];
    uint16_t key_data_length;
    uint8_t key_data[];
} eapol_key_packet_t;

/**
 * @see Ref: 802.11-2016 [12.7.2, Table 12-6]
 */
#define KEY_DATA_TYPE 0xdd

/**
 * @note Needs trailing byte due to casting to uint32_t and converting from netlong
 * @see Ref: 802.11-2016 [12.7.2, Table 12-6]
 */
#define KEY_DATA_OUI_IEEE80211 0x00fac00

/**
 * @see Ref: 802.11-2016 [12.7.2, Table 12-6]
 */
#define KEY_DATA_DATA_TYPE_PMKID_KDE 4

/**
 * @see Ref: 802.11-2016 [12.7.2]
 */
typedef struct __attribute__((__packed__)) {
    uint8_t type;
    uint8_t length;
    uint32_t oui:24;
    uint32_t data_type:8;
    uint8_t data[];
} key_data_field_t;

enum PMKIDState {
  WIFI_PMKID_STATE_HOME,
  WIFI_PMKID_STATE_SCANNING,
  WIFI_PMKID_STATE_SELECT_TARGET,
  WIFI_PMKID_STATE_RUNNING = 4,
};

struct WIFIPMKIDParams {};

struct PMKIDWebEventParams {
  const char* action;
  uint8_t channel;
  int ap_id;
};

class PMKIDApp {
  private:
    static void _task(void *taskParams);

  public:
    static uint8_t channel;
    static void deauth();
    static void send_packet(uint8_t* packet, uint16_t packetSize, int times=1);
    static void on_all_sta_disconnect();
    static void sendUpdate();
    static void sendState(int state);
    static void sendOutput(String output);
    static void webEvent(PMKIDWebEventParams *ps);
    static void on_rx_packet(void *buf, wifi_promiscuous_pkt_type_t type);
    static void scan();
    static void scan(void* params);
    static void stop_detect();
    static void start_detect();
    static void start_detect(void* params);
    static void ontouch(int pad, const char* type, uint32_t value);
    static void render();
};

#endif // PMKIDAPP_H
