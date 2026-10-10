#include "WeatherIconSelector.h"

#include "WeatherIconAssets.h"

namespace WeatherIconSelector {

const Bitmap1bpp& select(WeatherCondition condition, bool isDay) {
  using namespace WeatherIconAssets;

  switch (condition) {
    case WeatherCondition::CLEAR:
    case WeatherCondition::MAINLY_CLEAR:
      return isDay ? CLEAR_DAY : CLEAR_NIGHT;

    case WeatherCondition::PARTLY_CLOUDY:
      return isDay ? PARTLY_DAY : PARTLY_NIGHT;

    case WeatherCondition::OVERCAST:
      return OVERCAST;
    case WeatherCondition::FOG:
      return FOG;
    case WeatherCondition::DRIZZLE:
    case WeatherCondition::FREEZING_DRIZZLE:
      return DRIZZLE;
    case WeatherCondition::RAIN:
    case WeatherCondition::FREEZING_RAIN:
    case WeatherCondition::RAIN_SHOWERS:
      return RAIN;
    case WeatherCondition::SNOW:
    case WeatherCondition::SNOW_GRAINS:
    case WeatherCondition::SNOW_SHOWERS:
      return SNOW;
    case WeatherCondition::THUNDERSTORM:
    case WeatherCondition::THUNDERSTORM_HAIL:
      return THUNDER;
    case WeatherCondition::UNKNOWN:
      return UNKNOWN;
  }

  return UNKNOWN;
}

bool selfCheck() {
  struct Case {
    WeatherCondition condition;
    bool day;
    const Bitmap1bpp* expected;
  };

  using namespace WeatherIconAssets;
  static const Case CASES[] = {
      {WeatherCondition::UNKNOWN, true, &UNKNOWN},
      {WeatherCondition::UNKNOWN, false, &UNKNOWN},
      {WeatherCondition::CLEAR, true, &CLEAR_DAY},
      {WeatherCondition::CLEAR, false, &CLEAR_NIGHT},
      {WeatherCondition::MAINLY_CLEAR, true, &CLEAR_DAY},
      {WeatherCondition::MAINLY_CLEAR, false, &CLEAR_NIGHT},
      {WeatherCondition::PARTLY_CLOUDY, true, &PARTLY_DAY},
      {WeatherCondition::PARTLY_CLOUDY, false, &PARTLY_NIGHT},
      {WeatherCondition::OVERCAST, true, &OVERCAST},
      {WeatherCondition::FOG, true, &FOG},
      {WeatherCondition::DRIZZLE, true, &DRIZZLE},
      {WeatherCondition::FREEZING_DRIZZLE, true, &DRIZZLE},
      {WeatherCondition::RAIN, true, &RAIN},
      {WeatherCondition::FREEZING_RAIN, true, &RAIN},
      {WeatherCondition::SNOW, true, &SNOW},
      {WeatherCondition::SNOW_GRAINS, true, &SNOW},
      {WeatherCondition::RAIN_SHOWERS, true, &RAIN},
      {WeatherCondition::SNOW_SHOWERS, true, &SNOW},
      {WeatherCondition::THUNDERSTORM, true, &THUNDER},
      {WeatherCondition::THUNDERSTORM_HAIL, true, &THUNDER},
      {WeatherCondition::OVERCAST, false, &OVERCAST},
      {WeatherCondition::FOG, false, &FOG},
      {WeatherCondition::RAIN, false, &RAIN},
      {WeatherCondition::SNOW, false, &SNOW},
      {WeatherCondition::THUNDERSTORM, false, &THUNDER}
  };

  for (const Case& test : CASES) {
    const Bitmap1bpp& actual = select(test.condition, test.day);
    if (&actual != test.expected ||
        actual.width != 32 || actual.height != 32 ||
        actual.data == nullptr) {
      return false;
    }
  }
  // Unknown/unrecognized enum value must be safe.
  return &select(static_cast<WeatherCondition>(255), true) == &UNKNOWN;
}

}  // namespace WeatherIconSelector
