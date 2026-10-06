#pragma once

#include <cstring>

#include "WeatherService.h"

// Compare only values that Phase 7C-2 actually renders on the WEATHER page.
// Provider-only/raw fields must not cause unnecessary E-paper refreshes.
inline bool sameWeatherPageCurrent(
    const WeatherCurrentValue& a,
    const WeatherCurrentValue& b) {
  return strcmp(a.time, b.time) == 0 &&
         a.temperatureC == b.temperatureC &&
         a.condition == b.condition;
}

inline bool sameWeatherPageHourly(
    const WeatherHourlyValue& a,
    const WeatherHourlyValue& b) {
  return strcmp(a.time, b.time) == 0 &&
         a.temperatureC == b.temperatureC &&
         a.condition == b.condition &&
         a.precipitationProbability ==
             b.precipitationProbability;
}

inline bool sameWeatherPageTomorrow(
    const WeatherDailyValue& a,
    const WeatherDailyValue& b) {
  return strcmp(a.date, b.date) == 0 &&
         a.condition == b.condition &&
         a.temperatureMaxC == b.temperatureMaxC &&
         a.temperatureMinC == b.temperatureMinC &&
         a.precipitationProbabilityMax ==
             b.precipitationProbabilityMax;
}

inline bool sameWeatherPageContent(
    const WeatherSnapshot& a,
    const WeatherSnapshot& b) {
  if (!sameWeatherPageCurrent(a.current, b.current) ||
      !sameWeatherPageTomorrow(a.tomorrow, b.tomorrow)) {
    return false;
  }

  for (size_t index = 0;
       index < WeatherSnapshot::FUTURE_HOUR_COUNT;
       ++index) {
    if (!sameWeatherPageHourly(
            a.futureHours[index],
            b.futureHours[index])) {
      return false;
    }
  }

  return true;
}
