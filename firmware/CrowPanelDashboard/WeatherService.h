#pragma once

#include <Arduino.h>
#include <stddef.h>
#include <stdint.h>

#include "SecureHttpClient.h"
#include "WeatherCondition.h"

struct WeatherCurrentValue {
  static constexpr size_t TIME_CAPACITY = 20;

  char time[TIME_CAPACITY];
  float temperatureC;
  uint8_t weatherCode;
  WeatherCondition condition;
  bool isDay;
};

struct WeatherHourlyValue {
  static constexpr size_t TIME_CAPACITY = 20;

  char time[TIME_CAPACITY];
  float temperatureC;
  uint8_t weatherCode;
  WeatherCondition condition;
  uint8_t precipitationProbability;
  bool isDay;
};

struct WeatherDailyValue {
  static constexpr size_t DATE_CAPACITY = 12;

  char date[DATE_CAPACITY];
  uint8_t weatherCode;
  WeatherCondition condition;
  float temperatureMaxC;
  float temperatureMinC;
  uint8_t precipitationProbabilityMax;
};

struct WeatherSnapshot {
  static constexpr size_t FUTURE_HOUR_COUNT = 5;
  static constexpr size_t FUTURE_DAY_COUNT = 6; // tomorrow through day +6

  WeatherCurrentValue current;
  WeatherHourlyValue futureHours[FUTURE_HOUR_COUNT];
  WeatherDailyValue today;
  WeatherDailyValue tomorrow; // Phase 6/7 compatibility
  WeatherDailyValue futureDays[FUTURE_DAY_COUNT]; // [0] == tomorrow
  bool hasSixDayForecast; // false for the original 2-day endpoint
};

class WeatherService {
public:
  static constexpr size_t HOURLY_RESPONSE_COUNT = 6;
  static constexpr size_t DAILY_RESPONSE_COUNT = 2;
  static constexpr size_t SIX_DAY_RESPONSE_COUNT =
      WeatherSnapshot::FUTURE_DAY_COUNT + 1;

  explicit WeatherService(SecureHttpClient& httpClient);

  // Accepted legacy 2-day request used by Phase 6/7/8B. No behavior change.
  bool fetchLatest(double latitude, double longitude);
  // Opt-in 8C-1 7-day acquisition: today + six consecutive future days.
  bool fetchLatestSixDay(double latitude, double longitude);

  bool hasValidSnapshot() const;
  const WeatherSnapshot* lastValidSnapshot() const;
  const char* lastError() const;

private:
  bool fetchLatestInternal(double latitude, double longitude,
                           bool sevenDay);
  bool parseForecastPayload(const String& body,
                            WeatherSnapshot& parsed,
                            bool sevenDay);

  bool parseCurrentObject(const String& body,
                          int objectStart,
                          int objectEnd,
                          WeatherCurrentValue& current);
  bool parseHourlyObject(const String& body,
                         int objectStart,
                         int objectEnd,
                         WeatherSnapshot& parsed);
  bool parseDailyObject(const String& body,
                        int objectStart,
                        int objectEnd,
                        WeatherSnapshot& parsed,
                        bool sevenDay);

  void setError(const char* message);
  void setError(const String& message);

  SecureHttpClient& _httpClient;
  WeatherSnapshot _lastValid;
  bool _hasValidSnapshot;
  char _lastError[160];
};
