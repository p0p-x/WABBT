#ifndef BLESPAM_H
#define BLESPAM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "esp_mac.h"
#include "esp_gap_ble_api.h"
#include "esp_bt_main.h"
#include "esp_bt_defs.h"
#include "esp_bt.h"
#include "nvs.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
                                                    
typedef struct {
    uint8_t* data;
    size_t size;
} Packet;

Packet make_name_spam_packet(const char* name);
void stop_ble_name_spam();
void start_ble_name_spam(const char* name);
void ble_name_spam_loop(const char* name);

#ifdef __cplusplus
}
#endif

#endif // BLESPAM_H
