#include "ble/BLESpam.h"
#include "ble/BLESpamApp.h"
#include "BLEDevice.h"

#define TAG "BLENameSpam"

extern BLENameSpamState nameSpamState;

static esp_ble_adv_params_t name_spam_adv_params = {
  .adv_int_min        = 0x20,
  .adv_int_max        = 0x20,
  .adv_type           = ADV_TYPE_IND,
  .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
  //.peer_addr            =
  //.peer_addr_type       =
  .channel_map        = ADV_CHNL_ALL,
  .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

Packet make_name_spam_packet(const char* name) {
  uint8_t name_len = strlen(name);

  uint8_t raw_adv_data[] = {
    0x02, 0x01, 0x06,
    (uint8_t)(name_len + 1), 0x09,
    // NAME INSERT HERE
    0x03, 0x02, 0x12, 0x18,
    0x02, 0x0A, 0x00
  };

  Packet packet;
  packet.size = sizeof(raw_adv_data) + name_len;
  packet.data = (uint8_t*)malloc(packet.size);
  if (packet.data) {
    memcpy(packet.data, raw_adv_data, 5);
    memcpy(packet.data + 5, name, name_len);
    memcpy(packet.data + 5 + name_len, raw_adv_data + 5, sizeof(raw_adv_data) - 5);
  }
  return packet;
};

void stop_ble_name_spam() {
  BLEDevice::deinit();
}

void start_ble_name_spam(const char* name) {
  esp_err_t ret;

  uint8_t mac_address[6];
  srand((unsigned int)esp_timer_get_time());

  // Generate each byte of the MAC address
  for (int i = 0; i < 6; i++) {
    mac_address[i] = (uint8_t)rand() % 256;
  }

  ESP_ERROR_CHECK(esp_iface_mac_addr_set(mac_address, ESP_MAC_BT));

  BLEDevice::init("");

  esp_ble_gap_set_device_name(name);

  char randomChars[4];
  randomChars[0] = ' ';
  randomChars[1] = 'a' + (rand() % 26);
  randomChars[2] = 'A' + (rand() % 26);
  randomChars[3] = '\0';

  // Calculate the total length needed
  int len = strlen(name) + strlen(randomChars) + 1; // +1 for the null terminator

  // Allocate memory for the new string
  char* randomName = (char *)malloc(len);
  strcpy(randomName, name);
  strcat(randomName, randomChars);
  Packet packet = make_name_spam_packet(randomName);
  free(randomName);
   
  ret = esp_ble_gap_config_adv_data_raw(packet.data, packet.size);
  ret = esp_ble_gap_start_advertising(&name_spam_adv_params);
  nameSpamState = BLE_NAME_SPAM_STATE_RUNNING;
  free(packet.data);
}

void ble_name_spam_loop(const char* name) {
  while (1) {
    start_ble_name_spam(name);
    vTaskDelay(20 / portTICK_PERIOD_MS);
    stop_ble_name_spam();
  }
}