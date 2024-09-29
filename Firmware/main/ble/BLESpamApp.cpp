#include <ArduinoJson.h>

#include "ble/BLESpam.h"
#include "ble/BLESpamApp.h"
#include "DisplayManager.h"
#include "wifi/WebServerModule.h"

#define TAG "BLESpamApp"

char* tmpName = nullptr;
const char* name = "HACK THE PLAN8";

TaskHandle_t nameSpamTaskHandle = NULL;
TaskHandle_t nameSpamLEDTaskHandle = NULL;
BLENameSpamState nameSpamState = BLE_NAME_SPAM_STATE_HOME;

void BLENameSpamApp::stop_attack() {
  if (nameSpamTaskHandle != NULL) { 
    vTaskDelete(nameSpamTaskHandle);
    nameSpamTaskHandle = NULL;
  }

  if (nameSpamLEDTaskHandle != NULL) {
    vTaskDelete(nameSpamLEDTaskHandle);
    nameSpamLEDTaskHandle = NULL;
  }

  LEDStrip::animationRunning = false;

  if (tmpName != nullptr) {
    free(tmpName);
    tmpName = nullptr;
  }

  stop_ble_name_spam();

  leds->blink(255, 0, 0, 2);
  nameSpamState = BLE_NAME_SPAM_STATE_HOME;
};

void BLENameSpamApp::start_attack(const char* name) {
  NameSpamParams* params = new NameSpamParams{name};
  esp_err_t ret = xTaskCreate(&BLENameSpamApp::_task, "nspam", 10000, params, 2, &nameSpamTaskHandle);
  if (ret == ESP_FAIL) {
    ESP_LOGE(TAG, "nspam task err: %s", esp_err_to_name(ret));
  }
  xTaskCreate(&BLENameSpamApp::AnimationTask, "nspamleds", 4000, NULL, 3, &nameSpamLEDTaskHandle);
  LEDStrip::animationRunning = true;
  nameSpamState = BLE_NAME_SPAM_STATE_START;
};

void BLENameSpamApp::_task(void *taskParams) {
  NameSpamParams *params = static_cast<NameSpamParams*>(taskParams);
  ble_name_spam_loop(params->name);
};

void BLENameSpamApp::AnimationTask(void *params) {
  while (1) {
    leds->set_color(0, 0, 255, 0);
    leds->set_color(1, 0, 0, 0);
    vTaskDelay(300 / portTICK_PERIOD_MS);
    leds->set_color(0, 0, 0, 0);
    leds->set_color(1, 0, 255, 0);
    vTaskDelay(300 / portTICK_PERIOD_MS);
  }
};

void BLENameSpamApp::webEvent(NameSpamWebEventParams *ps) {
  ESP_LOGI(TAG, "Web: %s", ps->action);
  JsonDocument doc;
  String res;

  if (strcmp(ps->action, "status") == 0) {
    doc["app"] = APP_ID_BLE_NAMESPAM;
    doc["state"] = (int)nameSpamState;
    serializeJson(doc, res);
    return WebServerModule::notifyClients(res);
  }

  if (strcmp(ps->action, "start") == 0) {
    DisplayManager::appRunning = true;
    leds->stop_animations();
    bleApp = BLE_APP_SPAM;
    DisplayManager::setMenu(BLE_MENU);
    tmpName = (char*) malloc(strlen(ps->name) + 1);
    strcpy(tmpName, ps->name);
    BLENameSpamApp::start_attack(tmpName);
  }

  if (strcmp(ps->action, "stop") == 0) {
    BLENameSpamApp::stop_attack();
    DisplayManager::appRunning = false;
    bleApp = BLE_APP_NONE;
    DisplayManager::setMenu(MAIN_MENU);
    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }
};

void BLENameSpamApp::ontouch(int pad, const char* type, uint32_t value) {
  switch (pad) {
    case LEFT: {
      break;
    }
    case RIGHT: {
      break;
    }
    case MIDDLE: {
      if (strcmp(type, "longPress") == 0) {
        switch (nameSpamState) {
          case BLE_NAME_SPAM_STATE_HOME: {
            bleApp = BLE_APP_NONE;
            DisplayManager::appRunning = false;
            return;
          }
          case BLE_NAME_SPAM_STATE_START:
          case BLE_NAME_SPAM_STATE_RUNNING: {
            stop_attack();
            return;
          }
          default:
            break;
        }
      }

      if (nameSpamState == BLE_NAME_SPAM_STATE_HOME) {
        BLENameSpamApp::start_attack(name);
        return;
      }
      
      break;
    }
  }
};

void BLENameSpamApp::render() {
  display->text("main/ble/spam:", 0);
  display->text("", 1);

  switch (nameSpamState) {
    case BLE_NAME_SPAM_STATE_HOME: {
      display->centeredText("start", 2);
      break;
    }
    case BLE_NAME_SPAM_STATE_START: {
      display->centeredText("starting", 2);
      break;
    }
    case BLE_NAME_SPAM_STATE_RUNNING: {
      display->centeredText("running", 2);
      break;
    }
    default:
      break;
  }
  
  display->text("", 3);

  vTaskDelay(20 / portTICK_PERIOD_MS);
}