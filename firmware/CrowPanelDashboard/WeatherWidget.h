#pragma once

#include <Arduino.h>

#include "Bitmap1bpp.h"
#include "GraphicsBW.h"

class WeatherWidget {
public:
  WeatherWidget(GraphicsBW& graphics, int16_t x, int16_t y);

  bool render(const Bitmap1bpp& icon,
              const char* label,
              const char* temperature);

private:
  GraphicsBW& _graphics;
  int16_t _x;
  int16_t _y;
};
