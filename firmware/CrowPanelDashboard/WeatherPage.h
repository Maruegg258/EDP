#pragma once

#include "GraphicsBW.h"
#include "WeatherService.h"

class WeatherPage {
public:
  explicit WeatherPage(GraphicsBW& graphics);

  bool render(const WeatherSnapshot* snapshot);

private:
  bool renderNotReady();
  bool drawCentered(const char* text,
                    int16_t y,
                    uint8_t scale);
  static const char* shortConditionName(WeatherCondition condition);
  static bool formatHour(const char* isoTime,
                         char* output,
                         size_t capacity);
  static bool formatDate(const char* isoDate,
                         char* output,
                         size_t capacity);
  static bool formatTemperature(float temperatureC,
                                char* output,
                                size_t capacity);

  GraphicsBW& _graphics;
};
