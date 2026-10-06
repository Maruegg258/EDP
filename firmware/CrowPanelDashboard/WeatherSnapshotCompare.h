#pragma once

#include <cstring>

#include "WeatherService.h"

inline bool sameWeatherCurrent(
    const WeatherCurrentValue& a,
    const WeatherCurrentValue& b) {
  return strcmp(a.time, b.time) == 0 &&
         a.temperatureC == b.temperatureC &&
         a.weatherCode == b.weatherCode &&
         a.condition == b.condition &&
         a.isDay == b.isDay;
}

inline bool sameWeatherHourly(
    const WeatherHourlyValue& a,
    const WeatherHourlyValue& b) {
  return strcmp(a.time, b.time) == 0 &&
         a.temperatureC == b.temperatureC &&
         a.weatherCode == b.weatherCode &&
         a.condition == b.condition &&
         a.precipitationProbability == b.precipitationProbability &&
         a.isDay == b.isDay;
}

inline bool sameWeatherDaily(
    const WeatherDailyValue& a,
    const WeatherDailyValue& b) {
  return strcmp(a.date, b.date) == 0 &&
         a.weatherCode == b.weatherCode &&
         a.condition == b.condition &&
         a.temperatureMaxC == b.temperatureMaxC &&
         a.temperatureMinC == b.temperatureMinC &&
         a.precipitationProbabilityMax ==
             b.precipitationProbabilityMax;
}

inline bool sameWeatherSnapshot(
    const WeatherSnapshot& a,
    const WeatherSnapshot& b) {
  if (!sameWeatherCurrent(a.current, b.current) ||
      !sameWeatherDaily(a.today, b.today) ||
      !sameWeatherDaily(a.tomorrow, b.tomorrow)) {
    return false;
  }

  for (size_t index = 0;
       index < WeatherSnapshot::FUTURE_HOUR_COUNT;
       ++index) {
    if (!sameWeatherHourly(
            a.futureHours[index],
            b.futureHours[index])) {
      return false;
    }
  }

  return true;
}
