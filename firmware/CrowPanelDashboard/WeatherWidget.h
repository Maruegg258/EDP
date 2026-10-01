#pragma once

#include <Arduino.h>

#include "GraphicsBW.h"
#include "WidgetStates.h"

class WeatherWidget {
public:
  WeatherWidget(GraphicsBW& graphics, int16_t x, int16_t y);

  bool render(const WeatherWidgetState& state);

private:
  GraphicsBW& _graphics;
  int16_t _x;
  int16_t _y;
};
