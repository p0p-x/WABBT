#include "Display.h"
#include <cstring>
#include <string>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "font8x8_basic.h"
#include "esp_log.h"
#include "esp_err.h"

#define SDA 11
#define SDC 12
#define RESET -1
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define TAG "display"

Display::Display() {
  dev = new SSD1306_t;
  connected = false;
  setup();
}

void Display::setup() {
  i2c_master_init(dev, SDA, SDC, RESET);
  int ret = ssd1306_init(dev, SCREEN_WIDTH, SCREEN_HEIGHT);

  if (!ret) {
    connected = true;
    ESP_LOGI(TAG, "Created OLED Device");
    ssd1306_clear_screen(dev, false);
  } else {
    ESP_LOGE(TAG, "NO OLED Display Connected.");
  }
}

void Display::text(const char* text, int line) {
  if (!connected) {
    return;
  }

  if (text == NULL) {
    return;
  }
  std::string str(text); // Convert char* to std::string for easier manipulation
  if (str.length() < 16) {
    str.append(16 - str.length(), ' '); // Append spaces to make the length 16
  } else if (str.length() > 16) {
    str = str.substr(0, 16); // Truncate to 16 characters if longer
  }

  // Call to the actual display function using str.c_str() to get the C-string
  ssd1306_display_text(dev, line, const_cast<char*>(str.c_str()), 16, false);
};

void Display::centeredText(const char* text, int line) {
  if (!connected) {
    return;
  }

  if (text == NULL) {
    return;
  }

  std::string str(text);  // Convert char* to std::string for easier manipulation
  if (str.length() > 16) {
    str = str.substr(0, 16);  // Truncate to 16 characters if longer
  }

  int space = 16 - str.length();  // Calculate the total amount of padding needed
  if (space > 0) {
    int padLeft = space / 2;  // Padding for the left side
    int padRight = space - padLeft;  // Padding for the right side, ensures centering even if an odd number of spaces

    std::string paddedStr = std::string(padLeft, ' ') + str + std::string(padRight, ' ');  // Construct the centered string
    ssd1306_display_text(dev, line, const_cast<char*>(paddedStr.c_str()), 16, false);
  } else {
    // No padding needed, just display the text as it is
    ssd1306_display_text(dev, line, const_cast<char*>(str.c_str()), 16, false);
  }
};

void Display::clear() {
  ssd1306_clear_screen(dev, false);
  ssd1306_clear_screen(dev, false);
  ssd1306_clear_screen(dev, false);
}

void Display::fadeout() {
  if (!connected) {
    return;
  }

  ssd1306_fadeout(dev);
}

SSD1306_t* Display::get_dev() const {
  return dev;
}
