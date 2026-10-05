#include "WeatherCondition.h"

WeatherCondition weatherConditionFromWmoCode(uint8_t weatherCode) {
  switch (weatherCode) {
    case 0:
      return WeatherCondition::CLEAR;

    case 1:
      return WeatherCondition::MAINLY_CLEAR;

    case 2:
      return WeatherCondition::PARTLY_CLOUDY;

    case 3:
      return WeatherCondition::OVERCAST;

    case 45:
    case 48:
      return WeatherCondition::FOG;

    case 51:
    case 53:
    case 55:
      return WeatherCondition::DRIZZLE;

    case 56:
    case 57:
      return WeatherCondition::FREEZING_DRIZZLE;

    case 61:
    case 63:
    case 65:
      return WeatherCondition::RAIN;

    case 66:
    case 67:
      return WeatherCondition::FREEZING_RAIN;

    case 71:
    case 73:
    case 75:
      return WeatherCondition::SNOW;

    case 77:
      return WeatherCondition::SNOW_GRAINS;

    case 80:
    case 81:
    case 82:
      return WeatherCondition::RAIN_SHOWERS;

    case 85:
    case 86:
      return WeatherCondition::SNOW_SHOWERS;

    case 95:
    case 97:
      return WeatherCondition::THUNDERSTORM;

    case 96:
    case 99:
      return WeatherCondition::THUNDERSTORM_HAIL;

    default:
      return WeatherCondition::UNKNOWN;
  }
}

const char* weatherConditionName(WeatherCondition condition) {
  switch (condition) {
    case WeatherCondition::UNKNOWN:
      return "UNKNOWN";
    case WeatherCondition::CLEAR:
      return "CLEAR";
    case WeatherCondition::MAINLY_CLEAR:
      return "MAINLY_CLEAR";
    case WeatherCondition::PARTLY_CLOUDY:
      return "PARTLY_CLOUDY";
    case WeatherCondition::OVERCAST:
      return "OVERCAST";
    case WeatherCondition::FOG:
      return "FOG";
    case WeatherCondition::DRIZZLE:
      return "DRIZZLE";
    case WeatherCondition::FREEZING_DRIZZLE:
      return "FREEZING_DRIZZLE";
    case WeatherCondition::RAIN:
      return "RAIN";
    case WeatherCondition::FREEZING_RAIN:
      return "FREEZING_RAIN";
    case WeatherCondition::SNOW:
      return "SNOW";
    case WeatherCondition::SNOW_GRAINS:
      return "SNOW_GRAINS";
    case WeatherCondition::RAIN_SHOWERS:
      return "RAIN_SHOWERS";
    case WeatherCondition::SNOW_SHOWERS:
      return "SNOW_SHOWERS";
    case WeatherCondition::THUNDERSTORM:
      return "THUNDERSTORM";
    case WeatherCondition::THUNDERSTORM_HAIL:
      return "THUNDERSTORM_HAIL";
  }

  return "UNKNOWN";
}
