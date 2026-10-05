#include "WeatherWidgetMapper.h"

#include <cmath>
#include <cstdio>
#include <cstring>

#include "Icons.h"

namespace {

const Bitmap1bpp* iconForCondition(WeatherCondition condition) {
  switch (condition) {
    case WeatherCondition::CLEAR:
    case WeatherCondition::MAINLY_CLEAR:
      return &Icons::WEATHER_SUN;

    case WeatherCondition::PARTLY_CLOUDY:
    case WeatherCondition::OVERCAST:
    case WeatherCondition::FOG:
    case WeatherCondition::UNKNOWN:
      return &Icons::WEATHER_CLOUD;

    case WeatherCondition::DRIZZLE:
    case WeatherCondition::FREEZING_DRIZZLE:
    case WeatherCondition::RAIN:
    case WeatherCondition::FREEZING_RAIN:
    case WeatherCondition::SNOW:
    case WeatherCondition::SNOW_GRAINS:
    case WeatherCondition::RAIN_SHOWERS:
    case WeatherCondition::SNOW_SHOWERS:
    case WeatherCondition::THUNDERSTORM:
    case WeatherCondition::THUNDERSTORM_HAIL:
      return &Icons::WEATHER_RAIN;
  }

  return &Icons::WEATHER_CLOUD;
}

const char* labelForCondition(WeatherCondition condition) {
  switch (condition) {
    case WeatherCondition::UNKNOWN:
      return "UNKNOWN";
    case WeatherCondition::CLEAR:
      return "CLEAR";
    case WeatherCondition::MAINLY_CLEAR:
      return "MAINLY CLEAR";
    case WeatherCondition::PARTLY_CLOUDY:
      return "PARTLY CLOUDY";
    case WeatherCondition::OVERCAST:
      return "OVERCAST";
    case WeatherCondition::FOG:
      return "FOG";
    case WeatherCondition::DRIZZLE:
      return "DRIZZLE";
    case WeatherCondition::FREEZING_DRIZZLE:
      return "FRZ DRIZZLE";
    case WeatherCondition::RAIN:
      return "RAIN";
    case WeatherCondition::FREEZING_RAIN:
      return "FRZ RAIN";
    case WeatherCondition::SNOW:
      return "SNOW";
    case WeatherCondition::SNOW_GRAINS:
      return "SNOW GRAINS";
    case WeatherCondition::RAIN_SHOWERS:
      return "SHOWERS";
    case WeatherCondition::SNOW_SHOWERS:
      return "SNOW SHOWERS";
    case WeatherCondition::THUNDERSTORM:
      return "THUNDERSTORM";
    case WeatherCondition::THUNDERSTORM_HAIL:
      return "T-STORM HAIL";
  }

  return "UNKNOWN";
}

bool copyLabel(const char* source,
               char* destination,
               size_t capacity) {
  if (source == nullptr || destination == nullptr || capacity == 0) {
    return false;
  }

  const size_t length = strlen(source);

  if (length >= capacity) {
    return false;
  }

  memcpy(destination, source, length + 1);
  return true;
}

}  // namespace

bool buildWeatherWidgetPresentation(
    const WeatherCurrentValue& current,
    WeatherWidgetPresentation& presentation) {
  if (!std::isfinite(current.temperatureC)) {
    return false;
  }

  const Bitmap1bpp* icon = iconForCondition(current.condition);
  const char* label = labelForCondition(current.condition);

  if (icon == nullptr ||
      !copyLabel(
          label,
          presentation.label,
          sizeof(presentation.label))) {
    return false;
  }

  const int written = snprintf(
      presentation.temperature,
      sizeof(presentation.temperature),
      "%.1f C",
      static_cast<double>(current.temperatureC)
  );

  if (written < 0 ||
      static_cast<size_t>(written) >=
          sizeof(presentation.temperature)) {
    return false;
  }

  presentation.state.icon = icon;
  presentation.state.label = presentation.label;
  presentation.state.temperature = presentation.temperature;
  return true;
}


bool weatherWidgetMapperSelfCheck() {
  struct WidgetExpectation {
    WeatherCondition condition;
    const Bitmap1bpp* icon;
    const char* label;
  };

  static const WidgetExpectation EXPECTATIONS[] = {
    { WeatherCondition::UNKNOWN, &Icons::WEATHER_CLOUD, "UNKNOWN" },
    { WeatherCondition::CLEAR, &Icons::WEATHER_SUN, "CLEAR" },
    { WeatherCondition::MAINLY_CLEAR, &Icons::WEATHER_SUN, "MAINLY CLEAR" },
    { WeatherCondition::PARTLY_CLOUDY, &Icons::WEATHER_CLOUD, "PARTLY CLOUDY" },
    { WeatherCondition::OVERCAST, &Icons::WEATHER_CLOUD, "OVERCAST" },
    { WeatherCondition::FOG, &Icons::WEATHER_CLOUD, "FOG" },
    { WeatherCondition::DRIZZLE, &Icons::WEATHER_RAIN, "DRIZZLE" },
    { WeatherCondition::FREEZING_DRIZZLE, &Icons::WEATHER_RAIN, "FRZ DRIZZLE" },
    { WeatherCondition::RAIN, &Icons::WEATHER_RAIN, "RAIN" },
    { WeatherCondition::FREEZING_RAIN, &Icons::WEATHER_RAIN, "FRZ RAIN" },
    { WeatherCondition::SNOW, &Icons::WEATHER_RAIN, "SNOW" },
    { WeatherCondition::SNOW_GRAINS, &Icons::WEATHER_RAIN, "SNOW GRAINS" },
    { WeatherCondition::RAIN_SHOWERS, &Icons::WEATHER_RAIN, "SHOWERS" },
    { WeatherCondition::SNOW_SHOWERS, &Icons::WEATHER_RAIN, "SNOW SHOWERS" },
    { WeatherCondition::THUNDERSTORM, &Icons::WEATHER_RAIN, "THUNDERSTORM" },
    { WeatherCondition::THUNDERSTORM_HAIL, &Icons::WEATHER_RAIN, "T-STORM HAIL" }
  };

  for (const WidgetExpectation& entry : EXPECTATIONS) {
    WeatherCurrentValue current{};
    current.temperatureC = 23.2f;
    current.condition = entry.condition;

    WeatherWidgetPresentation presentation{};

    if (!buildWeatherWidgetPresentation(current, presentation) ||
        presentation.state.icon != entry.icon ||
        strcmp(presentation.state.label, entry.label) != 0 ||
        strcmp(presentation.state.temperature, "23.2 C") != 0) {
      return false;
    }
  }

  return true;
}
