#include "Arduino.h"
#include "BLEDevice.h"
#include "ble/BLEDroneApp.h"

#include "DisplayManager.h"

#define TAG "FakeDrone"

// Reference: https://github.com/sxjack/uav_electronic_ids/tree/main/id_open

int last_led = 0;
TaskHandle_t droneTaskHandle = NULL;
DroneState droneState = BLE_DRONE_STATE_HOME;

static UTM_Utilities         utm_utils;
static int    speed_kn = 40;
static float  x = 0.0, y = 0.0, z = 100.0, speed_m_x, max_dir_change = 75.0;
static double deg2rad = 0.0, m_deg_lat = 0.0, m_deg_long = 0.0;

void BLEDroneApp::sendOutput(String msg) {
  JsonDocument doc;
  String res;

  doc["app"] = APP_ID_BLE_DRONE;
  doc["output"] = msg;
  serializeJson(doc, res);
  WebServerModule::notifyClients(res);
};

void BLEDroneApp::sendState(int state) {
  JsonDocument doc;
  String res;

  doc["app"] = APP_ID_BLE_DRONE;
  doc["state"] = state;
  serializeJson(doc, res);
  WebServerModule::notifyClients(res);
};

void BLEDroneApp::stop() {
  droneState = BLE_DRONE_STATE_HOME;
  BLEDroneApp::sendState((int)droneState);

  leds->blink(255, 0, 0, 2);

  if (droneTaskHandle != NULL) { 
    vTaskDelete(droneTaskHandle);
    droneTaskHandle = NULL;
  }

  BLEDevice::deinit();
};

void BLEDroneApp::start(BLEDroneWebEventParams *ps) {
  esp_err_t ret = xTaskCreate(&BLEDroneApp::_task, "drone", 16000, ps, 1, &droneTaskHandle);
  if (ret == ESP_FAIL) {
    ESP_LOGE(TAG, "drone task err: %s", esp_err_to_name(ret));
  }
  droneState = BLE_DRONE_STATE_RUNNING;
  BLEDroneApp::sendState((int)droneState);
};

void BLEDroneApp::_task(void *taskParams) {
  BLEDroneWebEventParams *params = static_cast<BLEDroneWebEventParams*>(taskParams);

  uint8_t mac_address[6];
  srand((unsigned int)esp_timer_get_time());
  for (int i = 0; i < 6; i++) {
    mac_address[i] = (uint8_t)rand() % 256;
  }

  ESP_ERROR_CHECK(esp_iface_mac_addr_set(mac_address, ESP_MAC_BT));
  
  ID_OpenDrone          squitter;
  struct UTM_parameters utm_parameters;
  struct UTM_data       utm_data;
  double          lat_d = params->lat, long_d = params->lng;
  char *name = (char *)malloc(strlen(params->name) + 1);
  strcpy(name, params->name);

  ESP_LOGI(TAG, "Name: %s   lat: %f   lng: %f", name, lat_d, long_d);

  leds->blink(0, 255, 0);
  leds->set_color(0, 255, 0);

  char            text[64];
  time_t          time_2;
  struct tm       clock_tm;
  struct timeval  tv = {0,0};
  struct timezone utc = {0,0};
  deg2rad = (4.0 * atan(1.0)) / 180.0;

  memset(&clock_tm, 0, sizeof(struct tm));

  clock_tm.tm_hour  =  10;
  clock_tm.tm_mday  =  16;
  clock_tm.tm_mon   =  11;
  clock_tm.tm_year  = 122;

  tv.tv_sec =
  time_2    = mktime(&clock_tm);
  
  settimeofday(&tv, &utc);

  memset(&utm_parameters, 0, sizeof(utm_parameters));

  strncpy(utm_parameters.UAS_operator, name, ID_SIZE - 1);
  utm_parameters.UAS_operator[ID_SIZE - 1] = '\0';
  free(name);

  utm_parameters.region      = 0;
  utm_parameters.EU_category = 0;
  utm_parameters.EU_class    = 0;

  squitter.init(&utm_parameters);
  
  memset(&utm_data, 0, sizeof(utm_data));

  lat_d                   = 
  utm_data.latitude_d     =
  utm_data.base_latitude  = lat_d + (float)(rand() % 10 - 5) / 10000.0;
  long_d                  =
  utm_data.longitude_d    =
  utm_data.base_longitude =  long_d + (float)(rand() % 10 - 5) / 10000.0;
  utm_data.base_alt_m     = (float)rand() / (float)(RAND_MAX) * 2000.0;

  utm_data.alt_msl_m      = utm_data.base_alt_m + z;
  utm_data.alt_agl_m      = z;

  utm_data.speed_kn   = speed_kn;
  utm_data.satellites = 8;
  utm_data.base_valid = 1;

  speed_m_x = ((float) speed_kn) * 0.514444 / 5.0; // Because we update every 200 ms.

  utm_utils.calc_m_per_deg(lat_d, &m_deg_lat, &m_deg_long);

  int             dir_change;
  char            lat_s[16], long_s[16];
  float           rads, ran;
  uint32_t        msecs;
  static uint32_t last_update = 0;

  while (1) {

    msecs = millis();
    if ((msecs - last_update) > 199) {

      last_update = msecs;

      ran                  = 0.001 * (float) (((int) rand() % 1000) - 500);
      dir_change           = (int) (max_dir_change * ran);
      utm_data.heading     = (utm_data.heading + dir_change + 360) % 360;

      x                   += speed_m_x * sin(rads = (deg2rad * (float) utm_data.heading));
      y                   += speed_m_x * cos(rads);

      utm_data.latitude_d  = utm_data.base_latitude  + (y / m_deg_lat);
      utm_data.longitude_d = utm_data.base_longitude + (x / m_deg_long);

      dtostrf(utm_data.latitude_d, 10, 7, lat_s);
      dtostrf(utm_data.longitude_d, 10, 7, long_s);

      /**
      sprintf(text,"%s,%s,%d,%d,%d\r\n",
              lat_s,long_s,utm_data.heading,utm_data.speed_kn,dir_change);
      ESP_LOGI(TAG, "%s", text);
      */

      squitter.transmit(&utm_data);
    }

    vTaskDelay(200 / portTICK_PERIOD_MS);
  }
};

void BLEDroneApp::webEvent(BLEDroneWebEventParams *ps) {
  ESP_LOGI(TAG, "Web: %s", ps->action);
  JsonDocument doc;
  String res;

  if (strcmp(ps->action, "status") == 0) {
    doc["app"] = APP_ID_BLE_DRONE;
    doc["state"] = (int)droneState;
    serializeJson(doc, res);
    return WebServerModule::notifyClients(res);
  }

  if (strcmp(ps->action, "start") == 0) {
    DisplayManager::appRunning = true;
    leds->stop_animations();
    bleApp = BLE_APP_FAKE_DRONE;
    DisplayManager::setMenu(WIFI_MENU);
    BLEDroneApp::start(ps);
  }

  if (strcmp(ps->action, "stop") == 0) {
    BLEDroneApp::stop();
    DisplayManager::appRunning = false;
    bleApp = BLE_APP_NONE;
    DisplayManager::setMenu(MAIN_MENU);
    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }
};

void BLEDroneApp::ontouch(int pad, const char* type, uint32_t value) {
  switch (pad) {
    case LEFT: {
      break;
    }
    case RIGHT: {
      break;
    }
    case MIDDLE: {
      if (strcmp(type, "longPress") == 0) {
        switch (droneState) {
          case BLE_DRONE_STATE_HOME: {
            bleApp = BLE_APP_NONE;
            DisplayManager::appRunning = false;
            return;
          }
          case BLE_DRONE_STATE_RUNNING: {
            stop();
            return;
          }
          default:
            break;
        }
      }

      switch (droneState) {
        case BLE_DRONE_STATE_HOME: {
          BLEDroneWebEventParams ps = {
            .action = "start",
            .name = "HTP",
            .lat = 69,
            .lng = -4.20,
          };
          start(&ps);
          break;
        }
        default:
          break;
      }
      
      break;
    }
  }
};

void BLEDroneApp::render() {
  switch (droneState) {
    case BLE_DRONE_STATE_HOME: {
      display->centeredText("fake drone", 0);
      display->text("", 1);
      display->centeredText("start", 2);
      display->text("", 3);
      break;
    }
    case BLE_DRONE_STATE_RUNNING: {
      display->centeredText("fake drone", 0);
      display->text("", 1);
      display->centeredText("running", 2);
      display->text("", 3);
      break;
    }
    default:
      break;
  }

  vTaskDelay(20 / portTICK_PERIOD_MS);
};