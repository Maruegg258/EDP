#pragma once

#include <cstring>

#include "MarketDataService.h"

struct MarketPageState {
  char btcPrice[MarketPriceValue::PRICE_CAPACITY];
  char ethPrice[MarketPriceValue::PRICE_CAPACITY];
  char hypePrice[MarketPriceValue::PRICE_CAPACITY];
};

inline void initializeMarketPageState(
    MarketPageState& state) {
  strcpy(state.btcPrice, "--");
  strcpy(state.ethPrice, "--");
  strcpy(state.hypePrice, "--");
}

inline bool copyMarketPagePrice(
    char* destination,
    size_t capacity,
    const MarketPriceValue* value) {
  if (destination == nullptr || capacity == 0) {
    return false;
  }

  const char* source =
      value != nullptr ? value->price : "--";

  if (strlen(source) >= capacity) {
    return false;
  }

  strcpy(destination, source);
  return true;
}

inline bool buildMarketPageState(
    const MarketDataService& service,
    MarketPageState& state) {
  initializeMarketPageState(state);

  return copyMarketPagePrice(
             state.btcPrice,
             sizeof(state.btcPrice),
             service.lastValidValue("BTCUSDT")) &&
         copyMarketPagePrice(
             state.ethPrice,
             sizeof(state.ethPrice),
             service.lastValidValue("ETHUSDT")) &&
         copyMarketPagePrice(
             state.hypePrice,
             sizeof(state.hypePrice),
             service.lastValidValue("HYPEUSDT"));
}

inline bool sameMarketPageContent(
    const MarketPageState& a,
    const MarketPageState& b) {
  return strcmp(a.btcPrice, b.btcPrice) == 0 &&
         strcmp(a.ethPrice, b.ethPrice) == 0 &&
         strcmp(a.hypePrice, b.hypePrice) == 0;
}
