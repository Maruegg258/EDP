#include "WeatherPage.h"

#include <cstdio>
#include <cstring>

#include "Font5x7.h"

namespace {

constexpr int16_t BORDER_X = 8;
constexpr int16_t BORDER_Y = 8;
constexpr uint16_t BORDER_WIDTH = 776;
constexpr uint16_t BORDER_HEIGHT = 256;

constexpr int16_t CURRENT_DIVIDER_X = 220;
constexpr int16_t FORECAST_START_X = 232;
constexpr int16_t FORECAST_COLUMN_WIDTH = 108;
constexpr int16_t FORECAST_TOP_Y = 66;
constexpr int16_t FORECAST_BOTTOM_Y = 166;
constexpr int16_t TOMORROW_DIVIDER_Y = 176;

}  // namespace

WeatherPage::WeatherPage(GraphicsBW& graphics)
    : _graphics(graphics) {
}

bool WeatherPage::drawCentered(
    const char* text,
    int16_t y,
    uint8_t scale) {
  if (text == nullptr) {
    return false;
  }

  const uint16_t textWidth =
      _graphics.textWidth(Font5x7::FONT, text, scale);

  if (textWidth == 0 || textWidth > _graphics.width()) {
    return false;
  }

  const int16_t x = static_cast<int16_t>(
      (_graphics.width() - textWidth) / 2U
  );

  return _graphics.drawText(
      Font5x7::FONT,
      text,
      x,
      y,
      scale,
      true
  );
}

const char* WeatherPage::shortConditionName(
    WeatherCondition condition) {
  switch (condition) {
    case WeatherCondition::UNKNOWN:
      return "UNKNOWN";
    case WeatherCondition::CLEAR:
      return "CLEAR";
    case WeatherCondition::MAINLY_CLEAR:
      return "M CLEAR";
    case WeatherCondition::PARTLY_CLOUDY:
      return "P CLOUDY";
    case WeatherCondition::OVERCAST:
      return "OVERCAST";
    case WeatherCondition::FOG:
      return "FOG";
    case WeatherCondition::DRIZZLE:
      return "DRIZZLE";
    case WeatherCondition::FREEZING_DRIZZLE:
      return "FRZ DRZL";
    case WeatherCondition::RAIN:
      return "RAIN";
    case WeatherCondition::FREEZING_RAIN:
      return "FRZ RAIN";
    case WeatherCondition::SNOW:
      return "SNOW";
    case WeatherCondition::SNOW_GRAINS:
      return "GRAINS";
    case WeatherCondition::RAIN_SHOWERS:
      return "SHOWERS";
    case WeatherCondition::SNOW_SHOWERS:
      return "SNOW SHW";
    case WeatherCondition::THUNDERSTORM:
      return "T-STORM";
    case WeatherCondition::THUNDERSTORM_HAIL:
      return "T-STORM HAIL";
  }

  return "UNKNOWN";
}

bool WeatherPage::formatHour(
    const char* isoTime,
    char* output,
    size_t capacity) {
  if (isoTime == nullptr || output == nullptr ||
      capacity < 6 || strlen(isoTime) < 16) {
    return false;
  }

  output[0] = isoTime[11];
  output[1] = isoTime[12];
  output[2] = ':';
  output[3] = isoTime[14];
  output[4] = isoTime[15];
  output[5] = '\0';
  return true;
}

bool WeatherPage::formatDate(
    const char* isoDate,
    char* output,
    size_t capacity) {
  if (isoDate == nullptr || output == nullptr ||
      capacity < 6 || strlen(isoDate) < 10) {
    return false;
  }

  output[0] = isoDate[5];
  output[1] = isoDate[6];
  output[2] = '/';
  output[3] = isoDate[8];
  output[4] = isoDate[9];
  output[5] = '\0';
  return true;
}

bool WeatherPage::formatTemperature(
    float temperatureC,
    char* output,
    size_t capacity) {
  if (output == nullptr || capacity == 0) {
    return false;
  }

  const int written = snprintf(
      output,
      capacity,
      "%.1f C",
      static_cast<double>(temperatureC)
  );

  return written > 0 &&
         static_cast<size_t>(written) < capacity;
}

bool WeatherPage::renderNotReady() {
  _graphics.clear(true);
  _graphics.drawRect(
      BORDER_X,
      BORDER_Y,
      BORDER_WIDTH,
      BORDER_HEIGHT,
      true
  );

  if (!drawCentered("WEATHER", 68, 4)) {
    return false;
  }

  return drawCentered("DATA NOT READY", 146, 2);
}

bool WeatherPage::render(const WeatherSnapshot* snapshot) {
  if (snapshot == nullptr) {
    return renderNotReady();
  }

  char currentTemp[16];
  char currentTime[8];

  if (!formatTemperature(
          snapshot->current.temperatureC,
          currentTemp,
          sizeof(currentTemp)) ||
      !formatHour(
          snapshot->current.time,
          currentTime,
          sizeof(currentTime))) {
    return false;
  }

  _graphics.clear(true);
  _graphics.drawRect(
      BORDER_X,
      BORDER_Y,
      BORDER_WIDTH,
      BORDER_HEIGHT,
      true
  );

  if (!_graphics.drawText(
          Font5x7::FONT,
          "WEATHER",
          24,
          20,
          2,
          true)) {
    return false;
  }

  _graphics.drawLine(20, 52, 772, 52, true);
  _graphics.drawLine(
      CURRENT_DIVIDER_X,
      FORECAST_TOP_Y,
      CURRENT_DIVIDER_X,
      FORECAST_BOTTOM_Y,
      true
  );
  _graphics.drawLine(
      20,
      TOMORROW_DIVIDER_Y,
      772,
      TOMORROW_DIVIDER_Y,
      true
  );

  if (!_graphics.drawText(
          Font5x7::FONT,
          "NOW",
          24,
          72,
          2,
          true) ||
      !_graphics.drawText(
          Font5x7::FONT,
          currentTemp,
          24,
          102,
          3,
          true) ||
      !_graphics.drawText(
          Font5x7::FONT,
          shortConditionName(snapshot->current.condition),
          24,
          139,
          1,
          true) ||
      !_graphics.drawText(
          Font5x7::FONT,
          currentTime,
          24,
          154,
          1,
          true)) {
    return false;
  }

  for (size_t index = 0;
       index < WeatherSnapshot::FUTURE_HOUR_COUNT;
       ++index) {
    const WeatherHourlyValue& hour =
        snapshot->futureHours[index];

    const int16_t x = static_cast<int16_t>(
        FORECAST_START_X +
        static_cast<int16_t>(index) * FORECAST_COLUMN_WIDTH
    );

    if (index > 0) {
      _graphics.drawLine(
          static_cast<int16_t>(x - 8),
          FORECAST_TOP_Y,
          static_cast<int16_t>(x - 8),
          FORECAST_BOTTOM_Y,
          true
      );
    }

    char hourText[8];
    char tempText[16];
    char precipText[12];

    if (!formatHour(hour.time, hourText, sizeof(hourText)) ||
        !formatTemperature(
            hour.temperatureC,
            tempText,
            sizeof(tempText))) {
      return false;
    }

    const int precipWritten = snprintf(
        precipText,
        sizeof(precipText),
        "P %u%%",
        static_cast<unsigned int>(
            hour.precipitationProbability)
    );

    if (precipWritten <= 0 ||
        static_cast<size_t>(precipWritten) >=
            sizeof(precipText)) {
      return false;
    }

    if (!_graphics.drawText(
            Font5x7::FONT,
            hourText,
            x,
            72,
            1,
            true) ||
        !_graphics.drawText(
            Font5x7::FONT,
            tempText,
            x,
            96,
            2,
            true) ||
        !_graphics.drawText(
            Font5x7::FONT,
            shortConditionName(hour.condition),
            x,
            128,
            1,
            true) ||
        !_graphics.drawText(
            Font5x7::FONT,
            precipText,
            x,
            150,
            1,
            true)) {
      return false;
    }
  }

  char tomorrowDate[8];
  char tomorrowHigh[16];
  char tomorrowLow[16];
  char tomorrowPrecip[12];

  if (!formatDate(
          snapshot->tomorrow.date,
          tomorrowDate,
          sizeof(tomorrowDate)) ||
      !formatTemperature(
          snapshot->tomorrow.temperatureMaxC,
          tomorrowHigh,
          sizeof(tomorrowHigh)) ||
      !formatTemperature(
          snapshot->tomorrow.temperatureMinC,
          tomorrowLow,
          sizeof(tomorrowLow))) {
    return false;
  }

  const int tomorrowPrecipWritten = snprintf(
      tomorrowPrecip,
      sizeof(tomorrowPrecip),
      "P %u%%",
      static_cast<unsigned int>(
          snapshot->tomorrow.precipitationProbabilityMax)
  );

  if (tomorrowPrecipWritten <= 0 ||
      static_cast<size_t>(tomorrowPrecipWritten) >=
          sizeof(tomorrowPrecip)) {
    return false;
  }

  if (!_graphics.drawText(
          Font5x7::FONT,
          "TOMORROW",
          24,
          194,
          2,
          true) ||
      !_graphics.drawText(
          Font5x7::FONT,
          tomorrowDate,
          24,
          224,
          1,
          true) ||
      !_graphics.drawText(
          Font5x7::FONT,
          shortConditionName(snapshot->tomorrow.condition),
          170,
          194,
          2,
          true) ||
      !_graphics.drawText(
          Font5x7::FONT,
          "HI",
          430,
          192,
          1,
          true) ||
      !_graphics.drawText(
          Font5x7::FONT,
          tomorrowHigh,
          458,
          192,
          1,
          true) ||
      !_graphics.drawText(
          Font5x7::FONT,
          "LO",
          430,
          216,
          1,
          true) ||
      !_graphics.drawText(
          Font5x7::FONT,
          tomorrowLow,
          458,
          216,
          1,
          true) ||
      !_graphics.drawText(
          Font5x7::FONT,
          tomorrowPrecip,
          650,
          204,
          1,
          true)) {
    return false;
  }

  return true;
}
