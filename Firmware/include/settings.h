#ifndef SETTINGS_H
#define SETTINGS_H

#include "esp_mac.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_gap_ble_api.h"

#include "WiFi.h"

#define MAC_LEN 6

#ifdef __cplusplus

extern const char* version;
extern char badge_name[16];

// WiFi AP & Webserver
extern char WIFI_SSID[32];
extern char WIFI_PASS[32];

extern uint32_t TOUCH_THRESHOLD;

enum BADGE_APPS {
  APP_ID_NONE,
  APP_ID_WIFI_DEAUTH,
  APP_ID_BLE_NAMESPAM,
  APP_ID_BLE_GATTATTACK,
  APP_ID_SETTINGS_NAME,
  APP_ID_SETTINGS_WIFI,
  APP_ID_WIFI_DEAUTH_DETECT,
  APP_ID_BLE_FASTPAIR_SPAM,
  APP_ID_BLE_SPAM_DETECT,
  APP_ID_BLE_VHCI,
  APP_ID_BLE_EASYSETUP_SPAM,
  APP_ID_WIFI_PMKID,
  APP_ID_BLE_SKIMMER_DETECT,
  APP_ID_BLE_DRONE,
  APP_ID_BLE_SMART_TAG,
  APP_ID_BLE_REPLAY,
  APP_ID_SETTINGS_TOUCH_THRESHOLD,
  APP_ID_SETTINGS_RESTART,
};

extern "C" {
#endif

typedef void (*AllClientsDisconnectedCallback)(void);

// TOUCH PAD
#define RIGHT 1
#define LEFT 9
#define MIDDLE 6
#define LONG_TOUCH 600000

// in settings.cpp
extern uint8_t bt_mac_address[MAC_LEN];
extern uint8_t ap_mac_address[MAC_LEN];
extern uint8_t st_mac_address[MAC_LEN];
extern const uint8_t default_bt_mac_address[MAC_LEN];
extern const uint8_t default_ap_mac_address[MAC_LEN];
extern const uint8_t default_st_mac_address[MAC_LEN];
extern IPAddress local_IP;
extern IPAddress gateway;
extern IPAddress subnet;

// WIFI (No setup)
#define DEFAULT_WIFI_SSID "WABBIT_WIFI"
#define DEFAULT_WIFI_PASS "WABBT1234"

#define DEFAULT_TOUCH_THRESHOLD 6300

// nvs stuff
esp_err_t init_nvs();
esp_err_t write_string_to_nvs(const char* key, const char* value);
esp_err_t read_string_from_nvs(const char* key, char* buffer, size_t buffer_size);
esp_err_t write_uint32_to_nvs(const char* key, uint32_t value);
esp_err_t read_uint32_from_nvs(const char* key, uint32_t* value);
esp_err_t write_blob_to_nvs(const char* key, const void* data, size_t length);
esp_err_t read_blob_from_nvs(const char* key, void* data, size_t length);
void read_settings();

// LEDs
#define IDLETIMER3 3 * 1000000 // 3 seconds
#define IDLETIMER 10 * 1000000 // 10 seconds
#define LED_STRIP_BLINK_GPIO  16
#define LED_STRIP_LED_NUMBERS 2
#define LED_STRIP_RMT_RES_HZ  (10 * 1000 * 1000)

// Default Scan/Adv BLE Params
esp_ble_scan_params_t default_scan_params = {
  .scan_type = BLE_SCAN_TYPE_ACTIVE,
  .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
  .scan_filter_policy = BLE_SCAN_FILTER_ALLOW_ALL,
  .scan_interval = 0x50,
  .scan_window = 0x30,
  .scan_duplicate = BLE_SCAN_DUPLICATE_DISABLE
};

esp_ble_adv_params_t default_adv_params = {
  .adv_int_min        = 0x20,
  .adv_int_max        = 0x20,
  .adv_type           = ADV_TYPE_IND,
  .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
  //.peer_addr            =
  //.peer_addr_type       =
  .channel_map        = ADV_CHNL_ALL,
  .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

#ifdef __cplusplus
}
#endif

#endif // SETTINGS_H