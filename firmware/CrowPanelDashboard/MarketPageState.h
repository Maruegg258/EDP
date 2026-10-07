#pragma once

#include <Arduino.h>
#include <cstring>

struct MarketPageState {
  static constexpr size_t PRICE_CAPACITY = 24;

  char btcPrice[PRICE_CAPACITY];
  char ethPrice[PRICE_CAPACITY];
  char hypePrice[PRICE_CAPACITY];
};

inline void initializeMarketPageState(
    MarketPageState& state) {
  strcpy(state.btcPrice, "--");
  strcpy(state.ethPrice, "--");
  strcpy(state.hypePrice, "--");
}

inline bool sameMarketPageContent(
    const MarketPageState& a,
    const MarketPageState& b) {
  return strcmp(a.btcPrice, b.btcPrice) == 0 &&
         strcmp(a.ethPrice, b.ethPrice) == 0 &&
         strcmp(a.hypePrice, b.hypePrice) == 0;
}
