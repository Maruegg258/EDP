#include "Phase8WeatherPresentation.h"

#include <cmath>
#include <cstdio>
#include <cstring>

#include "WeatherIconSelector.h"

namespace {

const char* shortLabel(WeatherCondition condition, bool isDay) {
  switch (condition) {
    case WeatherCondition::UNKNOWN: return "UNKNOWN";
    case WeatherCondition::CLEAR: return isDay ? "SUNNY" : "CLEAR";
    case WeatherCondition::MAINLY_CLEAR: return "MAINLY CLR";
    case WeatherCondition::PARTLY_CLOUDY: return "PARTLY CLD";
    case WeatherCondition::OVERCAST: return "OVERCAST";
    case WeatherCondition::FOG: return "FOG";
    case WeatherCondition::DRIZZLE: return "DRIZZLE";
    case WeatherCondition::FREEZING_DRIZZLE: return "FRZ DRZL";
    case WeatherCondition::RAIN: return "RAIN";
    case WeatherCondition::FREEZING_RAIN: return "FRZ RAIN";
    case WeatherCondition::SNOW: return "SNOW";
    case WeatherCondition::SNOW_GRAINS: return "SNOW GRAIN";
    case WeatherCondition::RAIN_SHOWERS: return "SHOWERS";
    case WeatherCondition::SNOW_SHOWERS: return "SNOW SHWRS";
    case WeatherCondition::THUNDERSTORM: return "T-STORM";
    case WeatherCondition::THUNDERSTORM_HAIL: return "T-STORM HL";
  }
  return "UNKNOWN";
}

}  // namespace

namespace Phase8WeatherPresentation {

void setUnavailable(Phase8CurrentWeather& out) {
  out.icon = &WeatherIconSelector::select(WeatherCondition::UNKNOWN, true);
  strcpy(out.label, "NO DATA");
  strcpy(out.temperature, "-- C");
}

bool fromCurrent(const WeatherCurrentValue& current,
                 Phase8CurrentWeather& out) {
  if (!std::isfinite(current.temperatureC) ||
      current.temperatureC < -999.9f ||
      current.temperatureC > 999.9f) {
    return false;
  }

  const char* label = shortLabel(current.condition, current.isDay);
  const size_t n = strlen(label);
  if (n == 0 || n >= sizeof(out.label)) return false;

  Phase8CurrentWeather candidate{};
  candidate.icon = &WeatherIconSelector::select(
      current.condition, current.isDay);
  if (candidate.icon == nullptr ||
      candidate.icon->width != 32 ||
      candidate.icon->height != 32 ||
      candidate.icon->data == nullptr) {
    return false;
  }

  memcpy(candidate.label, label, n + 1);
  const int written = snprintf(
      candidate.temperature, sizeof(candidate.temperature),
      "%.1f C", static_cast<double>(current.temperatureC));
  if (written <= 0 || static_cast<size_t>(written) >= sizeof(candidate.temperature) ||
      written > 10) {
    return false;
  }

  out = candidate;  // Keep previous visible state if mapping fails.
  return true;
}

bool equal(const Phase8CurrentWeather& a, const Phase8CurrentWeather& b) {
  return a.icon == b.icon &&
         strcmp(a.label, b.label) == 0 &&
         strcmp(a.temperature, b.temperature) == 0;
}

bool selfCheck() {
  const WeatherCondition CONDITIONS[] = {
      WeatherCondition::UNKNOWN, WeatherCondition::CLEAR,
      WeatherCondition::MAINLY_CLEAR, WeatherCondition::PARTLY_CLOUDY,
      WeatherCondition::OVERCAST, WeatherCondition::FOG,
      WeatherCondition::DRIZZLE, WeatherCondition::FREEZING_DRIZZLE,
      WeatherCondition::RAIN, WeatherCondition::FREEZING_RAIN,
      WeatherCondition::SNOW, WeatherCondition::SNOW_GRAINS,
      WeatherCondition::RAIN_SHOWERS, WeatherCondition::SNOW_SHOWERS,
      WeatherCondition::THUNDERSTORM, WeatherCondition::THUNDERSTORM_HAIL
  };

  for (WeatherCondition condition : CONDITIONS) {
    for (int day = 0; day < 2; ++day) {
      WeatherCurrentValue current{};
      current.condition = condition;
      current.isDay = day != 0;
      current.temperatureC = 26.9f;
      Phase8CurrentWeather result{};
      if (!fromCurrent(current, result) ||
          strlen(result.label) > 10 ||
          strcmp(result.temperature, "26.9 C") != 0 ||
          result.icon != &WeatherIconSelector::select(condition, day != 0)) {
        return false;
      }
    }
  }

  WeatherCurrentValue day{};
  day.condition = WeatherCondition::CLEAR;
  day.isDay = true;
  day.temperatureC = 22.7f;
  Phase8CurrentWeather sun{};
  Phase8CurrentWeather moon{};
  if (!fromCurrent(day, sun) ||
      strcmp(sun.label, "SUNNY") != 0 ||
      strcmp(sun.temperature, "22.7 C") != 0) return false;
  day.isDay = false;
  if (!fromCurrent(day, moon) || equal(sun, moon) ||
      strcmp(moon.label, "CLEAR") != 0 ||
      sun.icon == moon.icon) return false;

  day.temperatureC = -2.5f;
  Phase8CurrentWeather belowZero{};
  if (!fromCurrent(day, belowZero) ||
      strcmp(belowZero.temperature, "-2.5 C") != 0) return false;

  Phase8CurrentWeather unchanged = belowZero;
  day.temperatureC = NAN;
  if (fromCurrent(day, unchanged) || !equal(unchanged, belowZero)) return false;

  day.temperatureC = 1.0e9f;
  if (fromCurrent(day, unchanged) || !equal(unchanged, belowZero)) return false;

  Phase8CurrentWeather placeholder{};
  setUnavailable(placeholder);
  return strcmp(placeholder.label, "NO DATA") == 0 &&
         strcmp(placeholder.temperature, "-- C") == 0 &&
         placeholder.icon != nullptr;
}

}  // namespace Phase8WeatherPresentation
