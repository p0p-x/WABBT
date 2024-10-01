#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/touch_pad.h"

#include "settings.h"
#include "LEDStrip.h"
#include "DisplayManager.h"
#include "TouchController.h"

#define TAG "touch"

extern LEDStrip* leds;

TouchController::TouchController(touch_pad_t l, touch_pad_t r, touch_pad_t m) 
  : touch_right(r)
  , touch_left(l)
  , touch_middle(m) {}

void TouchController::setup(std::function<void(int, const char*, uint32_t)> touchCallback) {
  touch_pad_init();
  touch_pad_config(touch_right);
  touch_pad_config(touch_left);
  touch_pad_config(touch_middle);
  touch_pad_set_voltage(TOUCH_HVOLT_2V4, TOUCH_PAD_LOW_VOLTAGE_THRESHOLD, TOUCH_HVOLT_ATTEN_1V5);

  touch_pad_denoise_t denoise = {
    .grade = TOUCH_PAD_DENOISE_BIT4,
    .cap_level = TOUCH_PAD_DENOISE_CAP_L3,
  };
  touch_pad_denoise_set_config(&denoise);
  touch_pad_denoise_enable();

  touch_pad_set_fsm_mode(TOUCH_FSM_MODE_TIMER);
  touch_pad_fsm_start();

  MyTouchReadParams* params = new MyTouchReadParams{this, touchCallback, TOUCH_THRESHOLD};
  vTaskDelay(1000 / portTICK_PERIOD_MS); // Sleep led touchpads fully init

  xTaskCreate(&TouchController::touch_read_task, "touch_pad_read_task", 4000, params, 3, NULL);
  ESP_LOGI(TAG, "Touch reading task created");
}

void TouchController::touch_read_task(void *pvParameter) {
  MyTouchReadParams *params = static_cast<MyTouchReadParams*>(pvParameter);
  uint32_t touch_value;
  int64_t touch_start_time_right = 0, touch_start_time_left = 0, touch_start_time_middle = 0;
  bool is_touching_right = false, is_touching_left = false, is_touching_middle = false;

  params->controller->lock = false;

  while (true) {
    params->controller->right = params->controller->handle_touch(params, 
      params->controller->touch_right, touch_value, is_touching_right, touch_start_time_right);
    params->controller->left = params->controller->handle_touch(params, 
      params->controller->touch_left, touch_value, is_touching_left, touch_start_time_left);
    params->controller->middle = params->controller->handle_touch(params,
      params->controller->touch_middle, touch_value, is_touching_middle, touch_start_time_middle);

    // all buttons pressed
    if (is_touching_left && is_touching_middle && is_touching_right) {
      int r = rand() % 256;
      int g = rand() % 256;
      int b = rand() % 256;
      leds->set_color(r, g, b);
      vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    // left + right
    if (is_touching_left && is_touching_right && !is_touching_middle) {
      leds->set_color(255, 0, 255);

      int64_t touch_duration = esp_timer_get_time() - touch_start_time_left;
      if (touch_duration >= LONG_TOUCH * 2.5) {
        params->controller->lock = !params->controller->lock;
        leds->blink(138, 0, 138, 5);
        ESP_LOGI(TAG, "Touch Pads: %s",
                params->controller->lock ? "LOCKED" : "UNLOCKED"
                );
      }
    }

    // left + middle
    if (is_touching_left && is_touching_middle && !is_touching_right) {
      leds->set_color(255, 255, 0);
    }

    // left only
    if (is_touching_left && !is_touching_right && !is_touching_middle) {
      leds->set_color(0, 255, 0, 0);
    }

    // right + middle
    if (is_touching_right && is_touching_middle && !is_touching_left) {
      leds->set_color(0, 255, 255);
    }

    // right only
    if (is_touching_right && !is_touching_left && !is_touching_middle) {
      leds->set_color(1, 0, 0, 255);
    }

    // middle only
    if (is_touching_middle && !is_touching_left && !is_touching_right) {
      leds->set_color(0, 255, 0);
      int64_t touch_duration = esp_timer_get_time() - touch_start_time_middle;
      if (touch_duration >= LONG_TOUCH) {
        leds->set_color(138, 0, 138);
      }
    }

    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

uint32_t TouchController::handle_touch(MyTouchReadParams* params, touch_pad_t touch_pad, uint32_t& touch_value, bool& is_touching, int64_t& start_time) {
  touch_pad_read_raw_data(touch_pad, &touch_value);
  if (touch_value > params->threshold) {
    if (LEDStrip::animationRunning) {
      leds->stop_animations();
    }

    if (!is_touching) {
      is_touching = true;
      start_time = esp_timer_get_time();
    }
  } else {
    if (is_touching) {
      int64_t touch_duration = esp_timer_get_time() - start_time;
      is_touching = false;
      if (touch_duration >= LONG_TOUCH) {
        params->callback(touch_pad, "longPress", touch_value);
      } else {
        params->callback(touch_pad, "shortPress", touch_value);
      }
    }
  }

  return touch_value;
}
