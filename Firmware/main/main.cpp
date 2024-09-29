#include "freertos/FreeRTOS.h"

#include "settings.h"
#include "DisplayManager.h"

#define TAG "main"

void setup() {
  DisplayManager::setup();
}

void loop() {
  DisplayManager::render();
  vTaskDelay(10 / portTICK_PERIOD_MS);
}
