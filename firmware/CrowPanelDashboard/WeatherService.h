#pragma once

#include <Arduino.h>
#include <stddef.h>
#include <stdint.h>

#include "SecureHttpClient.h"

struct WeatherCurrentValue {
  static constexpr size_t TIME_CAPACITY = 20;

  char time[TIME_CAPACITY];
  float temperatureC;
  uint8_t weatherCode;
  bool isDay;
};

struct WeatherHourlyValue {
  static constexpr size_t TIME_CAPACITY = 20;

  char time[TIME_CAPACITY];
  float temperatureC;
  uint8_t weatherCode;
  uint8_t precipitationProbability;
  bool isDay;
};

struct WeatherDailyValue {
  static constexpr size_t DATE_CAPACITY = 12;

  char date[DATE_CAPACITY];
  uint8_t weatherCode;
  float temperatureMaxC;
  float temperatureMinC;
  uint8_t precipitationProbabilityMax;
};

struct WeatherSnapshot {
  static constexpr size_t FUTURE_HOUR_COUNT = 5;

  WeatherCurrentValue current;
  WeatherHourlyValue futureHours[FUTURE_HOUR_COUNT];
  WeatherDailyValue today;
  WeatherDailyValue tomorrow;
};

class WeatherService {
public:
  static constexpr size_t HOURLY_RESPONSE_COUNT = 6;
  static constexpr size_t DAILY_RESPONSE_COUNT = 2;

  explicit WeatherService(SecureHttpClient& httpClient);

  bool fetchLatest(double latitude, double longitude);

  bool hasValidSnapshot() const;
  const WeatherSnapshot* lastValidSnapshot() const;
  const char* lastError() const;

private:
  bool parseForecastPayload(const String& body,
                            WeatherSnapshot& parsed);

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
                        WeatherSnapshot& parsed);

  void setError(const char* message);
  void setError(const String& message);

  SecureHttpClient& _httpClient;
  WeatherSnapshot _lastValid;
  bool _hasValidSnapshot;
  char _lastError[160];
};
