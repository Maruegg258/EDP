#pragma once

#include <stdint.h>

enum class WeatherCondition : uint8_t {
  UNKNOWN,
  CLEAR,
  MAINLY_CLEAR,
  PARTLY_CLOUDY,
  OVERCAST,
  FOG,
  DRIZZLE,
  FREEZING_DRIZZLE,
  RAIN,
  FREEZING_RAIN,
  SNOW,
  SNOW_GRAINS,
  RAIN_SHOWERS,
  SNOW_SHOWERS,
  THUNDERSTORM,
  THUNDERSTORM_HAIL
};

WeatherCondition weatherConditionFromWmoCode(uint8_t weatherCode);
const char* weatherConditionName(WeatherCondition condition);
