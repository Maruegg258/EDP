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
  static constexpr size_t TRACKED_SYMBOL_COUNT = 3;

  explicit MarketDataService(SecureHttpClient& httpClient);

  bool fetchLatest(const char* symbol);

  size_t trackedSymbolCount() const;
  const char* trackedSymbol(size_t index) const;

  bool hasValidValue(const char* symbol) const;
  const MarketPriceValue* lastValidValue(const char* symbol) const;
  const char* lastError(const char* symbol) const;

private:
  struct MarketSlot {
    const char* symbol;
    MarketPriceValue lastValid;
    bool hasValidValue;
    char lastError[128];
  };

  int findSlotIndex(const char* symbol) const;

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

  void setRequestError(const char* message);
  void setRequestError(const String& message);
  void setSlotError(size_t index, const char* message);
  void setSlotError(size_t index, const String& message);

  SecureHttpClient& _httpClient;
  MarketSlot _slots[TRACKED_SYMBOL_COUNT];
  char _requestError[128];
};
