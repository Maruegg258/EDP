#include "WeatherService.h"

#include <cmath>
#include <cstdlib>
#include <cstring>

namespace {

constexpr double MIN_TEMPERATURE_C = -100.0;
constexpr double MAX_TEMPERATURE_C = 100.0;

int skipWhitespace(const String& text, int position, int limit) {
  while (position < limit) {
    const char c = text[position];

    if (c != ' ' && c != '\t' && c != '\r' && c != '\n') {
      break;
    }

    ++position;
  }

  return position;
}

bool findNamedContainer(const String& text,
                        const char* key,
                        int searchStart,
                        int searchEnd,
                        char openChar,
                        char closeChar,
                        int& contentStart,
                        int& contentEnd) {
  if (key == nullptr || searchStart < 0 || searchEnd > text.length() ||
      searchStart >= searchEnd) {
    return false;
  }

  String needle = "\"";
  needle += key;
  needle += "\"";

  const int keyPosition = text.indexOf(needle, searchStart);

  if (keyPosition < 0 || keyPosition >= searchEnd) {
    return false;
  }

  int position = keyPosition + needle.length();
  position = skipWhitespace(text, position, searchEnd);

  if (position >= searchEnd || text[position] != ':') {
    return false;
  }

  position = skipWhitespace(text, position + 1, searchEnd);

  if (position >= searchEnd || text[position] != openChar) {
    return false;
  }

  const int openPosition = position;
  int depth = 0;
  bool inString = false;
  bool escaped = false;

  for (; position < searchEnd; ++position) {
    const char c = text[position];

    if (inString) {
      if (escaped) {
        escaped = false;
      } else if (c == '\\') {
        escaped = true;
      } else if (c == '"') {
        inString = false;
      }

      continue;
    }

    if (c == '"') {
      inString = true;
      continue;
    }

    if (c == openChar) {
      ++depth;
      continue;
    }

    if (c == closeChar) {
      --depth;

      if (depth == 0) {
        contentStart = openPosition + 1;
        contentEnd = position;
        return true;
      }

      if (depth < 0) {
        return false;
      }
    }
  }

  return false;
}

bool findValueStart(const String& text,
                    const char* key,
                    int searchStart,
                    int searchEnd,
                    int& valueStart) {
  if (key == nullptr || searchStart < 0 || searchEnd > text.length() ||
      searchStart >= searchEnd) {
    return false;
  }

  String needle = "\"";
  needle += key;
  needle += "\"";

  const int keyPosition = text.indexOf(needle, searchStart);

  if (keyPosition < 0 || keyPosition >= searchEnd) {
    return false;
  }

  int position = keyPosition + needle.length();
  position = skipWhitespace(text, position, searchEnd);

  if (position >= searchEnd || text[position] != ':') {
    return false;
  }

  position = skipWhitespace(text, position + 1, searchEnd);

  if (position >= searchEnd) {
    return false;
  }

  valueStart = position;
  return true;
}

bool parseStringAt(const String& text,
                   int valueStart,
                   int limit,
                   char* destination,
                   size_t capacity,
                   int* valueEnd = nullptr) {
  if (destination == nullptr || capacity == 0 ||
      valueStart < 0 || valueStart >= limit ||
      text[valueStart] != '"') {
    return false;
  }

  size_t written = 0;
  int position = valueStart + 1;

  while (position < limit) {
    const char c = text[position++];

    if (c == '"') {
      destination[written] = '\0';

      if (valueEnd != nullptr) {
        *valueEnd = position;
      }

      return true;
    }

    // Forecast timestamps/dates are plain ISO-8601 strings. Reject escaped
    // content rather than silently accepting a shape outside this contract.
    if (c == '\\' || written + 1 >= capacity) {
      return false;
    }

    destination[written++] = c;
  }

  return false;
}

bool parseDoubleAt(const String& text,
                   int valueStart,
                   int limit,
                   double& value,
                   int* valueEnd = nullptr) {
  if (valueStart < 0 || valueStart >= limit) {
    return false;
  }

  const char* base = text.c_str();
  char* end = nullptr;
  const double parsed = strtod(base + valueStart, &end);

  if (end == base + valueStart || end == nullptr) {
    return false;
  }

  const int endPosition = static_cast<int>(end - base);

  if (endPosition > limit || !std::isfinite(parsed)) {
    return false;
  }

  value = parsed;

  if (valueEnd != nullptr) {
    *valueEnd = endPosition;
  }

  return true;
}

bool parseIntegerAt(const String& text,
                    int valueStart,
                    int limit,
                    long& value,
                    int* valueEnd = nullptr) {
  if (valueStart < 0 || valueStart >= limit) {
    return false;
  }

  const char* base = text.c_str();
  char* end = nullptr;
  const long parsed = strtol(base + valueStart, &end, 10);

  if (end == base + valueStart || end == nullptr) {
    return false;
  }

  const int endPosition = static_cast<int>(end - base);

  if (endPosition > limit) {
    return false;
  }

  value = parsed;

  if (valueEnd != nullptr) {
    *valueEnd = endPosition;
  }

  return true;
}

bool validScalarTerminator(const String& text,
                           int valueEnd,
                           int objectEnd) {
  const int position =
      skipWhitespace(text, valueEnd, objectEnd);

  return position == objectEnd ||
         (position < objectEnd && text[position] == ',');
}

bool parseStringField(const String& text,
                      int objectStart,
                      int objectEnd,
                      const char* key,
                      char* destination,
                      size_t capacity) {
  int valueStart = 0;
  int valueEnd = 0;

  return findValueStart(
             text,
             key,
             objectStart,
             objectEnd,
             valueStart) &&
         parseStringAt(
             text,
             valueStart,
             objectEnd,
             destination,
             capacity,
             &valueEnd) &&
         validScalarTerminator(text, valueEnd, objectEnd);
}

bool parseDoubleField(const String& text,
                      int objectStart,
                      int objectEnd,
                      const char* key,
                      double& value) {
  int valueStart = 0;
  int valueEnd = 0;

  return findValueStart(
             text,
             key,
             objectStart,
             objectEnd,
             valueStart) &&
         parseDoubleAt(
             text,
             valueStart,
             objectEnd,
             value,
             &valueEnd) &&
         validScalarTerminator(text, valueEnd, objectEnd);
}

bool parseIntegerField(const String& text,
                       int objectStart,
                       int objectEnd,
                       const char* key,
                       long& value) {
  int valueStart = 0;
  int valueEnd = 0;

  return findValueStart(
             text,
             key,
             objectStart,
             objectEnd,
             valueStart) &&
         parseIntegerAt(
             text,
             valueStart,
             objectEnd,
             value,
             &valueEnd) &&
         validScalarTerminator(text, valueEnd, objectEnd);
}

bool consumeArraySeparator(const String& text,
                           int& position,
                           int arrayEnd,
                           bool expectMore) {
  position = skipWhitespace(text, position, arrayEnd);

  if (expectMore) {
    if (position >= arrayEnd || text[position] != ',') {
      return false;
    }

    position = skipWhitespace(text, position + 1, arrayEnd);
    return position < arrayEnd;
  }

  return position == arrayEnd;
}

template <size_t COUNT, size_t CAPACITY>
bool parseStringArray(const String& text,
                      int objectStart,
                      int objectEnd,
                      const char* key,
                      char (&values)[COUNT][CAPACITY]) {
  int arrayStart = 0;
  int arrayEnd = 0;

  if (!findNamedContainer(
          text,
          key,
          objectStart,
          objectEnd,
          '[',
          ']',
          arrayStart,
          arrayEnd)) {
    return false;
  }

  int position = skipWhitespace(text, arrayStart, arrayEnd);

  for (size_t index = 0; index < COUNT; ++index) {
    int valueEnd = 0;

    if (!parseStringAt(
            text,
            position,
            arrayEnd,
            values[index],
            CAPACITY,
            &valueEnd)) {
      return false;
    }

    position = valueEnd;

    if (!consumeArraySeparator(
            text,
            position,
            arrayEnd,
            index + 1 < COUNT)) {
      return false;
    }
  }

  return true;
}

template <size_t COUNT>
bool parseDoubleArray(const String& text,
                      int objectStart,
                      int objectEnd,
                      const char* key,
                      double (&values)[COUNT]) {
  int arrayStart = 0;
  int arrayEnd = 0;

  if (!findNamedContainer(
          text,
          key,
          objectStart,
          objectEnd,
          '[',
          ']',
          arrayStart,
          arrayEnd)) {
    return false;
  }

  int position = skipWhitespace(text, arrayStart, arrayEnd);

  for (size_t index = 0; index < COUNT; ++index) {
    int valueEnd = 0;

    if (!parseDoubleAt(
            text,
            position,
            arrayEnd,
            values[index],
            &valueEnd)) {
      return false;
    }

    position = valueEnd;

    if (!consumeArraySeparator(
            text,
            position,
            arrayEnd,
            index + 1 < COUNT)) {
      return false;
    }
  }

  return true;
}

template <size_t COUNT>
bool parseIntegerArray(const String& text,
                       int objectStart,
                       int objectEnd,
                       const char* key,
                       long (&values)[COUNT]) {
  int arrayStart = 0;
  int arrayEnd = 0;

  if (!findNamedContainer(
          text,
          key,
          objectStart,
          objectEnd,
          '[',
          ']',
          arrayStart,
          arrayEnd)) {
    return false;
  }

  int position = skipWhitespace(text, arrayStart, arrayEnd);

  for (size_t index = 0; index < COUNT; ++index) {
    int valueEnd = 0;

    if (!parseIntegerAt(
            text,
            position,
            arrayEnd,
            values[index],
            &valueEnd)) {
      return false;
    }

    position = valueEnd;

    if (!consumeArraySeparator(
            text,
            position,
            arrayEnd,
            index + 1 < COUNT)) {
      return false;
    }
  }

  return true;
}

bool isDigit(char c) {
  return c >= '0' && c <= '9';
}

bool allDigits(const char* text, size_t start, size_t end) {
  for (size_t index = start; index < end; ++index) {
    if (!isDigit(text[index])) {
      return false;
    }
  }

  return true;
}

int parseTwoDigits(const char* text, size_t offset) {
  return (text[offset] - '0') * 10 + (text[offset + 1] - '0');
}

bool validDate(const char* text) {
  if (text == nullptr || strlen(text) != 10 ||
      text[4] != '-' || text[7] != '-' ||
      !allDigits(text, 0, 4) ||
      !allDigits(text, 5, 7) ||
      !allDigits(text, 8, 10)) {
    return false;
  }

  const int month = parseTwoDigits(text, 5);
  const int day = parseTwoDigits(text, 8);

  return month >= 1 && month <= 12 &&
         day >= 1 && day <= 31;
}

bool validDateTime(const char* text, bool requireTopOfHour) {
  if (text == nullptr || strlen(text) != 16 ||
      text[4] != '-' || text[7] != '-' ||
      text[10] != 'T' || text[13] != ':' ||
      !allDigits(text, 0, 4) ||
      !allDigits(text, 5, 7) ||
      !allDigits(text, 8, 10) ||
      !allDigits(text, 11, 13) ||
      !allDigits(text, 14, 16)) {
    return false;
  }

  char date[11];
  memcpy(date, text, 10);
  date[10] = '\0';

  if (!validDate(date)) {
    return false;
  }

  const int hour = parseTwoDigits(text, 11);
  const int minute = parseTwoDigits(text, 14);

  if (hour < 0 || hour > 23 || minute < 0 || minute > 59) {
    return false;
  }

  return !requireTopOfHour || minute == 0;
}

bool validTemperature(double value) {
  return std::isfinite(value) &&
         value >= MIN_TEMPERATURE_C &&
         value <= MAX_TEMPERATURE_C;
}

bool validWeatherCode(long value) {
  // Preserve the bounded raw provider code. Documented WMO values are
  // normalized by WeatherCondition; undefined values remain UNKNOWN.
  return value >= 0 && value <= 99;
}

bool validProbability(long value) {
  return value >= 0 && value <= 100;
}

bool validIsDay(long value) {
  return value == 0 || value == 1;
}

void copyText(char* destination,
              size_t capacity,
              const char* source) {
  if (destination == nullptr || capacity == 0 || source == nullptr) {
    return;
  }

  strncpy(destination, source, capacity - 1);
  destination[capacity - 1] = '\0';
}


bool isLeapYear(int year) {
  return (year % 4 == 0 && year % 100 != 0) ||
         (year % 400 == 0);
}

int calendarMonthDays(int year, int month) {
  static constexpr uint8_t DAYS[] = {
      31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
  };
  if (month < 1 || month > 12) return 0;
  return month == 2 && isLeapYear(year) ? 29 : DAYS[month - 1];
}

int fourDigits(const char* date) {
  return (date[0] - '0') * 1000 + (date[1] - '0') * 100 +
         (date[2] - '0') * 10 + (date[3] - '0');
}

bool validCalendarDate(const char* date) {
  if (!validDate(date)) return false;
  const int year = fourDigits(date);
  const int month = parseTwoDigits(date, 5);
  const int day = parseTwoDigits(date, 8);
  return year >= 2000 && year <= 9999 &&
         day >= 1 && day <= calendarMonthDays(year, month);
}

bool nextCalendarDay(const char* previous, const char* current) {
  if (!validCalendarDate(previous) || !validCalendarDate(current)) {
    return false;
  }
  int year = fourDigits(previous);
  int month = parseTwoDigits(previous, 5);
  int day = parseTwoDigits(previous, 8) + 1;
  if (day > calendarMonthDays(year, month)) {
    day = 1;
    ++month;
    if (month == 13) { month = 1; ++year; }
  }
  return year == fourDigits(current) &&
         month == parseTwoDigits(current, 5) &&
         day == parseTwoDigits(current, 8);
}

// The existing scalar/array readers are reused by BOTH endpoint contracts.
// Provider arrays must have exactly 2 or 7 values; no partial/misaligned
// forecast is allowed to replace the last-valid WeatherSnapshot.
template <size_t COUNT>
bool parseValidatedDailySeries(const String& body,
                               int objectStart, int objectEnd,
                               WeatherSnapshot& parsed) {
  static_assert(COUNT == 2 ||
                    COUNT == WeatherService::SIX_DAY_RESPONSE_COUNT,
                "daily parser must receive 2 or 7 dates");
  char dates[COUNT][WeatherDailyValue::DATE_CAPACITY] = {};
  double maxC[COUNT] = {};
  double minC[COUNT] = {};
  long codes[COUNT] = {};
  long probability[COUNT] = {};

  if (!parseStringArray(body, objectStart, objectEnd, "time", dates) ||
      !parseIntegerArray(body, objectStart, objectEnd, "weather_code", codes) ||
      !parseDoubleArray(body, objectStart, objectEnd,
                        "temperature_2m_max", maxC) ||
      !parseDoubleArray(body, objectStart, objectEnd,
                        "temperature_2m_min", minC) ||
      !parseIntegerArray(body, objectStart, objectEnd,
                         "precipitation_probability_max", probability)) {
    return false;
  }

  if (strncmp(parsed.current.time, dates[0], 10) != 0) return false;

  for (size_t i = 0; i < COUNT; ++i) {
    if (!validCalendarDate(dates[i]) ||
        !validTemperature(maxC[i]) ||
        !validTemperature(minC[i]) ||
        maxC[i] < minC[i] ||
        !validWeatherCode(codes[i]) ||
        !validProbability(probability[i])) {
      return false;
    }
    if (i > 0 && (COUNT == WeatherService::SIX_DAY_RESPONSE_COUNT
                     ? !nextCalendarDay(dates[i-1], dates[i])
                     : strcmp(dates[i-1], dates[i]) == 0)) {
      return false;
    }
  }

  for (size_t i = 0; i < COUNT; ++i) {
    WeatherDailyValue* dest = (i == 0)
        ? &parsed.today
        : &parsed.futureDays[i - 1];
    copyText(dest->date, sizeof(dest->date), dates[i]);
    dest->weatherCode = static_cast<uint8_t>(codes[i]);
    dest->condition = weatherConditionFromWmoCode(dest->weatherCode);
    dest->temperatureMaxC = static_cast<float>(maxC[i]);
    dest->temperatureMinC = static_cast<float>(minC[i]);
    dest->precipitationProbabilityMax = static_cast<uint8_t>(probability[i]);
    if (i == 1) parsed.tomorrow = *dest;
  }

  parsed.hasSixDayForecast =
      (COUNT == WeatherService::SIX_DAY_RESPONSE_COUNT);
  return true;
}

}  // namespace

bool WeatherService::sixDayDateSelfCheck() {
  return nextCalendarDay("2026-10-11", "2026-10-12") &&
         nextCalendarDay("2026-10-31", "2026-11-01") &&
         nextCalendarDay("2026-12-31", "2027-01-01") &&
         nextCalendarDay("2028-02-28", "2028-02-29") &&
         nextCalendarDay("2028-02-29", "2028-03-01") &&
         nextCalendarDay("2027-02-28", "2027-03-01") &&
         !validCalendarDate("2027-02-29") &&
         !validCalendarDate("2026-04-31") &&
         !nextCalendarDay("2026-10-11", "2026-10-13") &&
         !nextCalendarDay("2026-10-11", "2026-10-11");
}

WeatherService::WeatherService(SecureHttpClient& httpClient)
    : _httpClient(httpClient),
      _lastValid{},
      _hasValidSnapshot(false) {
  _lastError[0] = '\0';
}

bool WeatherService::fetchLatest(double latitude, double longitude) {
  return fetchLatestInternal(latitude, longitude, false);
}

bool WeatherService::fetchLatestSixDay(double latitude, double longitude) {
  return fetchLatestInternal(latitude, longitude, true);
}

bool WeatherService::fetchLatestInternal(double latitude, double longitude,
                                         bool sevenDay) {
  if (!std::isfinite(latitude) || !std::isfinite(longitude) ||
      latitude < -90.0 || latitude > 90.0 ||
      longitude < -180.0 || longitude > 180.0) {
    setError("invalid weather coordinates");
    return false;
  }

  String url;
  url.reserve(512);
  url = "https://api.open-meteo.com/v1/forecast?latitude=";
  url += String(latitude, 6);
  url += "&longitude=";
  url += String(longitude, 6);
  url += "&current=temperature_2m,weather_code,is_day";
  url += "&hourly=temperature_2m,weather_code,precipitation_probability,is_day";
  url += "&daily=weather_code,temperature_2m_max,temperature_2m_min,precipitation_probability_max";
  url += "&forecast_hours=6";
  url += sevenDay ? "&forecast_days=7" : "&forecast_days=2";
  url += "&timezone=Asia%2FTaipei";

  SecureHttpResponse response;

  if (!_httpClient.get(url.c_str(), response)) {
    String error = "HTTPS fetch failed";

    if (response.statusCode != 0) {
      error += ": HTTP status ";
      error += response.statusCode;
    }

    if (response.error.length() > 0) {
      error += ": ";
      error += response.error;
    }

    setError(error);
    return false;
  }

  WeatherSnapshot parsed{};

  if (!parseForecastPayload(response.body, parsed, sevenDay)) {
    return false;
  }

  _lastValid = parsed;
  _hasValidSnapshot = true;
  setError("");
  return true;
}

bool WeatherService::hasValidSnapshot() const {
  return _hasValidSnapshot;
}

const WeatherSnapshot* WeatherService::lastValidSnapshot() const {
  return _hasValidSnapshot ? &_lastValid : nullptr;
}

const char* WeatherService::lastError() const {
  return _lastError;
}

bool WeatherService::parseForecastPayload(const String& body,
                                          WeatherSnapshot& parsed,
                                          bool sevenDay) {
  int currentStart = 0;
  int currentEnd = 0;
  int hourlyStart = 0;
  int hourlyEnd = 0;
  int dailyStart = 0;
  int dailyEnd = 0;

  if (!findNamedContainer(
          body,
          "current",
          0,
          body.length(),
          '{',
          '}',
          currentStart,
          currentEnd)) {
    setError("weather payload missing current object");
    return false;
  }

  if (!findNamedContainer(
          body,
          "hourly",
          0,
          body.length(),
          '{',
          '}',
          hourlyStart,
          hourlyEnd)) {
    setError("weather payload missing hourly object");
    return false;
  }

  if (!findNamedContainer(
          body,
          "daily",
          0,
          body.length(),
          '{',
          '}',
          dailyStart,
          dailyEnd)) {
    setError("weather payload missing daily object");
    return false;
  }

  WeatherSnapshot candidate{};

  if (!parseCurrentObject(
          body,
          currentStart,
          currentEnd,
          candidate.current)) {
    return false;
  }

  if (!parseHourlyObject(
          body,
          hourlyStart,
          hourlyEnd,
          candidate)) {
    return false;
  }

  if (!parseDailyObject(
          body,
          dailyStart,
          dailyEnd,
          candidate,
          sevenDay)) {
    return false;
  }

  parsed = candidate;
  return true;
}

bool WeatherService::parseCurrentObject(
    const String& body,
    int objectStart,
    int objectEnd,
    WeatherCurrentValue& current) {
  double temperature = 0.0;
  long weatherCode = 0;
  long isDay = 0;

  if (!parseStringField(
          body,
          objectStart,
          objectEnd,
          "time",
          current.time,
          sizeof(current.time)) ||
      !validDateTime(current.time, false)) {
    setError("invalid current weather time");
    return false;
  }

  if (!parseDoubleField(
          body,
          objectStart,
          objectEnd,
          "temperature_2m",
          temperature) ||
      !validTemperature(temperature)) {
    setError("invalid current temperature");
    return false;
  }

  if (!parseIntegerField(
          body,
          objectStart,
          objectEnd,
          "weather_code",
          weatherCode) ||
      !validWeatherCode(weatherCode)) {
    setError("invalid current weather code");
    return false;
  }

  if (!parseIntegerField(
          body,
          objectStart,
          objectEnd,
          "is_day",
          isDay) ||
      !validIsDay(isDay)) {
    setError("invalid current is_day value");
    return false;
  }

  current.temperatureC = static_cast<float>(temperature);
  current.weatherCode = static_cast<uint8_t>(weatherCode);
  current.condition =
      weatherConditionFromWmoCode(current.weatherCode);
  current.isDay = isDay == 1;
  return true;
}

bool WeatherService::parseHourlyObject(
    const String& body,
    int objectStart,
    int objectEnd,
    WeatherSnapshot& parsed) {
  char times[HOURLY_RESPONSE_COUNT][WeatherHourlyValue::TIME_CAPACITY] = {};
  double temperatures[HOURLY_RESPONSE_COUNT] = {};
  long weatherCodes[HOURLY_RESPONSE_COUNT] = {};
  long precipitationProbabilities[HOURLY_RESPONSE_COUNT] = {};
  long isDayValues[HOURLY_RESPONSE_COUNT] = {};

  if (!parseStringArray(
          body,
          objectStart,
          objectEnd,
          "time",
          times) ||
      !parseDoubleArray(
          body,
          objectStart,
          objectEnd,
          "temperature_2m",
          temperatures) ||
      !parseIntegerArray(
          body,
          objectStart,
          objectEnd,
          "weather_code",
          weatherCodes) ||
      !parseIntegerArray(
          body,
          objectStart,
          objectEnd,
          "precipitation_probability",
          precipitationProbabilities) ||
      !parseIntegerArray(
          body,
          objectStart,
          objectEnd,
          "is_day",
          isDayValues)) {
    setError("invalid or mis-sized hourly weather arrays");
    return false;
  }

  for (size_t index = 0; index < HOURLY_RESPONSE_COUNT; ++index) {
    if (!validDateTime(times[index], true) ||
        !validTemperature(temperatures[index]) ||
        !validWeatherCode(weatherCodes[index]) ||
        !validProbability(precipitationProbabilities[index]) ||
        !validIsDay(isDayValues[index])) {
      setError("hourly weather value failed validation");
      return false;
    }
  }

  // Current conditions can be sampled inside the hour (for example 23:30),
  // while hourly[0] is the top-of-hour row (23:00). Require only the same
  // YYYY-MM-DDTHH prefix; do not require equal temperature or minute.
  if (strncmp(parsed.current.time, times[0], 13) != 0) {
    setError("current weather hour does not align with hourly[0]");
    return false;
  }

  for (size_t index = 0;
       index < WeatherSnapshot::FUTURE_HOUR_COUNT;
       ++index) {
    const size_t sourceIndex = index + 1;
    WeatherHourlyValue& destination = parsed.futureHours[index];

    copyText(
        destination.time,
        sizeof(destination.time),
        times[sourceIndex]);
    destination.temperatureC =
        static_cast<float>(temperatures[sourceIndex]);
    destination.weatherCode =
        static_cast<uint8_t>(weatherCodes[sourceIndex]);
    destination.condition =
        weatherConditionFromWmoCode(destination.weatherCode);
    destination.precipitationProbability =
        static_cast<uint8_t>(
            precipitationProbabilities[sourceIndex]);
    destination.isDay = isDayValues[sourceIndex] == 1;
  }

  return true;
}

bool WeatherService::parseDailyObject(
    const String& body,
    int objectStart,
    int objectEnd,
    WeatherSnapshot& parsed,
    bool sevenDay) {
  const bool valid = sevenDay
      ? parseValidatedDailySeries<SIX_DAY_RESPONSE_COUNT>(
            body, objectStart, objectEnd, parsed)
      : parseValidatedDailySeries<DAILY_RESPONSE_COUNT>(
            body, objectStart, objectEnd, parsed);
  if (!valid) {
    setError("daily arrays invalid, mis-sized, or nonconsecutive dates");
    return false;
  }
  return true;
}

void WeatherService::setError(const char* message) {
  if (message == nullptr) {
    _lastError[0] = '\0';
    return;
  }

  strncpy(_lastError, message, sizeof(_lastError) - 1);
  _lastError[sizeof(_lastError) - 1] = '\0';
}

void WeatherService::setError(const String& message) {
  setError(message.c_str());
}
