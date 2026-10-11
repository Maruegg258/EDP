#pragma once

#include <stddef.h>

// Phase 8B-3: independently owned strings. The dashboard does not retain
// pointers to MarketPriceValue or a temporary formatter buffer.
struct Phase8MarketPrices {
  static constexpr size_t DISPLAY_CAPACITY = 11;  // <=10 native 34px glyphs
  char eth[DISPLAY_CAPACITY];
  char btc[DISPLAY_CAPACITY];
  char hype[DISPLAY_CAPACITY];
};

namespace Phase8MarketPresentation {
void setUnavailable(Phase8MarketPrices& out);
bool equal(const Phase8MarketPrices& a, const Phase8MarketPrices& b);
// Format a Binance fixed-decimal ASCII price without floating-point loss.
// Symbol policies: BTC 1dp, ETH 2dp, HYPE 3dp; half-up rounding.
// Drop decimal precision as needed to fit 10 visible glyphs, never truncate
// the integer or silently use scientific notation.
// Invalid/unrepresentable prices leave destination untouched.
bool formatPrice(const char* symbol, const char* source,
                 char* destination, size_t capacity);
bool selfCheck();
}  // namespace Phase8MarketPresentation
