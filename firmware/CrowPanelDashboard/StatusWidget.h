#pragma once

#include <Arduino.h>

#include "GraphicsBW.h"
#include "WidgetStates.h"

class StatusWidget {
public:
  StatusWidget(GraphicsBW& graphics,
               int16_t x,
               int16_t y,
               uint16_t width,
               uint16_t height);

  bool render(const StatusWidgetState& state);

private:
  GraphicsBW& _graphics;
  int16_t _x;
  int16_t _y;
  uint16_t _width;
  uint16_t _height;
};
