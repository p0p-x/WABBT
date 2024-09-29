#ifndef DISPLAY_H
#define DISPLAY_H

#include "ssd1306.h"

class Display {
private:
  SSD1306_t* dev;

public:
  Display();
  bool connected;

  void setup();
  void text(const char* text, int line = 0);
  void centeredText(const char* text, int line);
  void clear();
  void fadeout();
  void update();
  void animations();
  SSD1306_t* get_dev() const;
};

#endif // DISPLAY_H
