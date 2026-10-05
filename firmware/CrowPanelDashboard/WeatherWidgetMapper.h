#pragma once

#include <stddef.h>

#include "WeatherService.h"
#include "WidgetStates.h"

struct WeatherWidgetPresentation {
  static constexpr size_t LABEL_CAPACITY = 16;
  static constexpr size_t TEMPERATURE_CAPACITY = 16;

  char label[LABEL_CAPACITY];
  char temperature[TEMPERATURE_CAPACITY];
  WeatherWidgetState state;
};

bool buildWeatherWidgetPresentation(
    const WeatherCurrentValue& current,
    WeatherWidgetPresentation& presentation);

bool weatherWidgetMapperSelfCheck();
