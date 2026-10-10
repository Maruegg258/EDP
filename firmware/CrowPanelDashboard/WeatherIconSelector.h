#pragma once

#include "Bitmap1bpp.h"
#include "WeatherCondition.h"

// Phase 8A-2 candidate selection only; production WeatherWidgetMapper remains
// unchanged until after on-panel artwork approval.
namespace WeatherIconSelector {
const Bitmap1bpp& select(WeatherCondition condition, bool isDay);
bool selfCheck();
}  // namespace WeatherIconSelector
