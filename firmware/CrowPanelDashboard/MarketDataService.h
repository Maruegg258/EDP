#pragma once

#include <Arduino.h>
#include <stddef.h>
#include <stdint.h>

#include "SecureHttpClient.h"

struct MarketPriceValue {
  static constexpr size_t SYMBOL_CAPACITY = 16;
  static constexpr size_t PRICE_CAPACITY = 24;

  char symbol[SYMBOL_CAPACITY];
  char price[PRICE_CAPACITY];
  uint64_t sourceTime;
  bool hasSourceTime;
};

class MarketDataService {
public:
  explicit MarketDataService(SecureHttpClient& httpClient);

  bool fetchLatest(const char* symbol);

  bool hasValidValue() const;
  const MarketPriceValue& lastValidValue() const;
  const char* lastError() const;

private:
  bool parseTickerPayload(const String& body,
                          const char* requestedSymbol,
                          MarketPriceValue& parsed);
  bool extractJsonString(const String& body,
                         const char* key,
                         char* destination,
                         size_t capacity) const;
  bool extractOptionalJsonUint64(const String& body,
                                 const char* key,
                                 uint64_t& value,
                                 bool& present) const;
  bool validatePositivePrice(const char* price) const;

  void setError(const char* message);
  void setError(const String& message);

  SecureHttpClient& _httpClient;
  MarketPriceValue _lastValid;
  bool _hasValidValue;
  char _lastError[128];
};
