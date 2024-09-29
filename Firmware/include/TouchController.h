#ifndef TOUCHCONTROLLER_H
#define TOUCHCONTROLLER_H

#include <functional>
#include <inttypes.h>

#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"

class TouchController;

// Renaming TouchReadParams to avoid naming conflict
struct MyTouchReadParams {
  TouchController* controller;  // Include the controller instance
  std::function<void(int, const char*, uint32_t)> callback;
  uint32_t threshold;
};

class TouchController {
public:
  uint32_t right;
  uint32_t left;
  uint32_t middle;
  TouchController(touch_pad_t l, touch_pad_t r, touch_pad_t m);
  void setup(std::function<void(int, const char*, uint32_t)> touchCallback);
  
private:
  touch_pad_t touch_right;
  touch_pad_t touch_left;
  touch_pad_t touch_middle;

  static void touch_read_task(void *pvParameter);
  uint32_t handle_touch(MyTouchReadParams* params, touch_pad_t touch_pad, uint32_t& touch_value, bool& is_touching, int64_t& start_time);
};

#endif // TOUCHCONTROLLER_H
