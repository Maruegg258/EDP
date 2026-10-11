#pragma once

#include "Bitmap1bpp.h"
#include "WeatherService.h"

// Phase 8C-2: bounded, owned, display-only six future days.
// The immutable bitmap pointers refer to approved native 32x32 artwork.
struct Phase8SixDayColumn {
  const Bitmap1bpp* icon;
  char weekday[4];     // e.g. MON
  char summary[18];   // e.g. "29/25C 80%" = max/min Celsius + precip %
};
struct Phase8SixDayForecast {
  static constexpr size_t COUNT = WeatherSnapshot::FUTURE_DAY_COUNT;
  Phase8SixDayColumn days[COUNT];
  bool available;
};

namespace Phase8SixDayPresentation {
void setUnavailable(Phase8SixDayForecast& out);
// Only validated WeatherService seven-day snapshots may render actual values.
// Returns false without overwriting the caller's prior output.
bool fromSnapshot(const WeatherSnapshot& snapshot,
                  Phase8SixDayForecast& out);
bool equal(const Phase8SixDayForecast& a, const Phase8SixDayForecast& b);
bool selfCheck();
}
