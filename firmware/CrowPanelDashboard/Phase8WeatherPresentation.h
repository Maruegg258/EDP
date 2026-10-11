#pragma once

#include <stddef.h>

#include "Bitmap1bpp.h"
#include "WeatherService.h"

// Phase 8B-4, only what the accepted Dashboard currently displays.
// Own label/temperature text; artwork points to immutable global 32x32 assets.
struct Phase8CurrentWeather {
  const Bitmap1bpp* icon;
  char label[11];       // <=10 native 17px glyphs
  char temperature[12]; // native 14px, e.g. "-2.5 C"
};

namespace Phase8WeatherPresentation {
void setUnavailable(Phase8CurrentWeather& out);
bool fromCurrent(const WeatherCurrentValue& current,
                 Phase8CurrentWeather& out);
bool equal(const Phase8CurrentWeather& a, const Phase8CurrentWeather& b);
bool selfCheck();
}  // namespace Phase8WeatherPresentation
