#include "Phase8MarketPresentation.h"

#include <cstring>

namespace {

constexpr size_t MAX_SOURCE_BYTES = 24;
constexpr size_t MAX_GLYPHS = 10;

int precisionForSymbol(const char* symbol) {
  if (symbol == nullptr) return -1;
  if (strcmp(symbol, "BTCUSDT") == 0) return 1;
  if (strcmp(symbol, "ETHUSDT") == 0) return 2;
  if (strcmp(symbol, "HYPEUSDT") == 0) return 3;
  return -1;
}

// Parses the plain decimal string returned by MarketDataService. Do not
// convert to binary float: an up-rounding carry must be predictable.
bool parseDecimal(const char* input,
                  char (&integer)[MAX_SOURCE_BYTES],
                  size_t& integerLength,
                  char (&fraction)[MAX_SOURCE_BYTES],
                  size_t& fractionLength) {
  if (input == nullptr) return false;
  const size_t length = strnlen(input, MAX_SOURCE_BYTES);
  if (length == 0 || length >= MAX_SOURCE_BYTES) return false;

  size_t digitsBefore = 0;
  fractionLength = 0;
  bool seenPoint = false;
  bool anyDigit = false;
  bool positive = false;

  for (size_t i = 0; i < length; ++i) {
    const char ch = input[i];
    if (ch == '.') {
      if (seenPoint) return false;
      seenPoint = true;
      continue;
    }
    if (ch < '0' || ch > '9') return false;
    anyDigit = true;
    positive |= (ch != '0');
    if (seenPoint) {
      fraction[fractionLength++] = ch;
    } else {
      integer[digitsBefore++] = ch;
    }
  }
  if (!anyDigit || !positive) return false;

  // Remove redundant leading zeros, without ever shortening magnitude.
  size_t first = 0;
  while (first < digitsBefore && integer[first] == '0') ++first;
  if (first == digitsBefore) {
    integer[0] = '0';
    integerLength = 1;
  } else {
    integerLength = digitsBefore - first;
    memmove(integer, integer + first, integerLength);
  }
  integer[integerLength] = '\0';
  fraction[fractionLength] = '\0';
  return true;
}

bool roundInto(const char* integer, size_t integerLength,
               const char* fraction, size_t fractionLength,
               size_t places, char (&output)[Phase8MarketPrices::DISPLAY_CAPACITY]) {
  const size_t initialLength = integerLength + (places ? (1 + places) : 0);
  if (initialLength > MAX_GLYPHS) return false;

  memcpy(output, integer, integerLength);
  if (places > 0) {
    output[integerLength] = '.';
    for (size_t i = 0; i < places; ++i) {
      output[integerLength + 1 + i] = i < fractionLength ? fraction[i] : '0';
    }
  }
  output[initialLength] = '\0';

  if (fractionLength <= places || fraction[places] < '5') return true;

  // Decimal half-up: propagate carry across '9' and the decimal separator.
  for (size_t step = initialLength; step > 0; --step) {
    char& digit = output[step - 1];
    if (digit == '.') continue;
    if (digit == '9') {
      digit = '0';
      continue;
    }
    ++digit;
    return true;
  }

  // All integer/fractional digits were nine. Retry with fewer places if the
  // carry would overflow the width; reject if even integer cannot fit.
  if (initialLength + 1 > MAX_GLYPHS) return false;
  memmove(output + 1, output, initialLength + 1);
  output[0] = '1';
  return true;
}

bool test(const char* symbol, const char* src, const char* expected) {
  char out[Phase8MarketPrices::DISPLAY_CAPACITY] = "SENTINEL";
  return Phase8MarketPresentation::formatPrice(
             symbol, src, out, sizeof(out)) &&
         strcmp(out, expected) == 0;
}

}  // namespace

namespace Phase8MarketPresentation {

void setUnavailable(Phase8MarketPrices& out) {
  strcpy(out.eth, "--");
  strcpy(out.btc, "--");
  strcpy(out.hype, "--");
}

bool equal(const Phase8MarketPrices& a, const Phase8MarketPrices& b) {
  return strcmp(a.eth, b.eth) == 0 &&
         strcmp(a.btc, b.btc) == 0 &&
         strcmp(a.hype, b.hype) == 0;
}

bool formatPrice(const char* symbol, const char* source,
                 char* destination, size_t capacity) {
  const int preferred = precisionForSymbol(symbol);
  if (preferred < 0 || destination == nullptr ||
      capacity < Phase8MarketPrices::DISPLAY_CAPACITY) return false;

  char integer[MAX_SOURCE_BYTES]{};
  char fraction[MAX_SOURCE_BYTES]{};
  size_t nInteger = 0, nFraction = 0;
  if (!parseDecimal(source, integer, nInteger, fraction, nFraction) ||
      nInteger > MAX_GLYPHS) return false;

  size_t places = static_cast<size_t>(preferred);
  if (nInteger + 1 + places > MAX_GLYPHS) {
    places = nInteger < MAX_GLYPHS ? MAX_GLYPHS - nInteger - 1 : 0;
  }
  for (;;) {
    char candidate[Phase8MarketPrices::DISPLAY_CAPACITY]{};
    if (roundInto(integer, nInteger, fraction, nFraction, places, candidate)) {
      const size_t n = strlen(candidate);
      if (n <= MAX_GLYPHS && n + 1 <= capacity) {
        memcpy(destination, candidate, n + 1);
        return true;
      }
    }
    if (places == 0) break;
    --places;
  }
  return false;
}

bool selfCheck() {
  if (!test("BTCUSDT", "82750.10000000", "82750.1") ||
      !test("ETHUSDT", "2493.56000000", "2493.56") ||
      !test("HYPEUSDT", "84.15400000", "84.154") ||
      !test("BTCUSDT", "999999999.96", "1000000000") ||
      !test("ETHUSDT", "99999999.995", "100000000") ||
      !test("HYPEUSDT", "0.00049", "0.000") ||
      !test("HYPEUSDT", "9.9999", "10.000") ||
      !test("ETHUSDT", "0000002493.565", "2493.57") ||
      !test("BTCUSDT", "999999999.4", "999999999") ||
      !test("BTCUSDT", ".5", "0.5")) {
    return false;
  }

  static const char* const REJECT[] = {
      "", "0", "000.000", "-5.0", "NaN", "1e8", "123x", "12..3",
      "9999999999.9", "10000000000", "123456789012345678901234"
  };
  for (const char* s : REJECT) {
    char unchanged[Phase8MarketPrices::DISPLAY_CAPACITY] = "SAFE";
    if (formatPrice("BTCUSDT", s, unchanged, sizeof(unchanged)) ||
        strcmp(unchanged, "SAFE") != 0) return false;
  }
  char unchanged[Phase8MarketPrices::DISPLAY_CAPACITY] = "SAFE";
  if (formatPrice("WRONG", "1.0", unchanged, sizeof(unchanged)) ||
      strcmp(unchanged, "SAFE") != 0) return false;
  char tiny[4] = "OK";
  if (formatPrice("BTCUSDT", "1.0", tiny, sizeof(tiny)) ||
      strcmp(tiny, "OK") != 0) return false;

  Phase8MarketPrices prices{};
  setUnavailable(prices);
  Phase8MarketPrices copy = prices;
  if (!equal(prices, copy)) return false;
  strcpy(copy.eth, "2493.56");
  return !equal(prices, copy);
}

}  // namespace Phase8MarketPresentation
