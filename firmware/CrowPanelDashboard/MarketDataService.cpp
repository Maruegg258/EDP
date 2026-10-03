#include "MarketDataService.h"

#include <cmath>
#include <cstdlib>
#include <cstring>

namespace {

constexpr char BINANCE_FUTURES_TICKER_PREFIX[] =
    "https://fapi.binance.com/fapi/v2/ticker/price?symbol=";

bool isJsonWhitespace(char value) {
  return value == ' ' ||
         value == '\t' ||
         value == '\r' ||
         value == '\n';
}

}  // namespace

MarketDataService::MarketDataService(SecureHttpClient& httpClient)
    : _httpClient(httpClient),
      _hasValidValue(false) {
  memset(&_lastValid, 0, sizeof(_lastValid));
  _lastError[0] = '\0';
}

bool MarketDataService::fetchLatest(const char* symbol) {
  _lastError[0] = '\0';

  if (symbol == nullptr || symbol[0] == '\0') {
    setError("market symbol is empty");
    return false;
  }

  if (strlen(symbol) >= MarketPriceValue::SYMBOL_CAPACITY) {
    setError("market symbol exceeds service capacity");
    return false;
  }

  String url(BINANCE_FUTURES_TICKER_PREFIX);
  url += symbol;

  SecureHttpResponse response;

  if (!_httpClient.get(url.c_str(), response)) {
    String error("HTTPS fetch failed");

    if (response.error.length() > 0) {
      error += ": ";
      error += response.error;
    }

    setError(error);
    return false;
  }

  MarketPriceValue parsed = {};

  if (!parseTickerPayload(response.body, symbol, parsed)) {
    return false;
  }

  _lastValid = parsed;
  _hasValidValue = true;
  return true;
}

bool MarketDataService::hasValidValue() const {
  return _hasValidValue;
}

const MarketPriceValue& MarketDataService::lastValidValue() const {
  return _lastValid;
}

const char* MarketDataService::lastError() const {
  return _lastError;
}

bool MarketDataService::parseTickerPayload(
    const String& body,
    const char* requestedSymbol,
    MarketPriceValue& parsed) {
  if (body.length() == 0) {
    setError("ticker response body is empty");
    return false;
  }

  if (!extractJsonString(
          body,
          "symbol",
          parsed.symbol,
          sizeof(parsed.symbol))) {
    setError("ticker response has invalid or missing symbol");
    return false;
  }

  if (strcmp(parsed.symbol, requestedSymbol) != 0) {
    setError("ticker response symbol does not match request");
    return false;
  }

  if (!extractJsonString(
          body,
          "price",
          parsed.price,
          sizeof(parsed.price))) {
    setError("ticker response has invalid or missing price");
    return false;
  }

  if (!validatePositivePrice(parsed.price)) {
    setError("ticker price is not a finite positive number");
    return false;
  }

  if (!extractOptionalJsonUint64(
          body,
          "time",
          parsed.sourceTime,
          parsed.hasSourceTime)) {
    setError("ticker response time field is invalid");
    return false;
  }

  return true;
}

bool MarketDataService::extractJsonString(
    const String& body,
    const char* key,
    char* destination,
    size_t capacity) const {
  if (key == nullptr || destination == nullptr || capacity == 0) {
    return false;
  }

  String token("\"");
  token += key;
  token += "\"";

  const int keyPosition = body.indexOf(token);
  if (keyPosition < 0) {
    return false;
  }

  int cursor = keyPosition + token.length();

  while (cursor < body.length() &&
         isJsonWhitespace(body[cursor])) {
    ++cursor;
  }

  if (cursor >= body.length() || body[cursor] != ':') {
    return false;
  }

  ++cursor;

  while (cursor < body.length() &&
         isJsonWhitespace(body[cursor])) {
    ++cursor;
  }

  if (cursor >= body.length() || body[cursor] != '"') {
    return false;
  }

  ++cursor;
  size_t written = 0;

  while (cursor < body.length()) {
    const char value = body[cursor++];

    if (value == '"') {
      if (written == 0) {
        return false;
      }

      destination[written] = '\0';
      return true;
    }

    if (value == '\\' || written + 1 >= capacity) {
      return false;
    }

    destination[written++] = value;
  }

  return false;
}

bool MarketDataService::extractOptionalJsonUint64(
    const String& body,
    const char* key,
    uint64_t& value,
    bool& present) const {
  present = false;
  value = 0;

  if (key == nullptr) {
    return false;
  }

  String token("\"");
  token += key;
  token += "\"";

  const int keyPosition = body.indexOf(token);
  if (keyPosition < 0) {
    return true;
  }

  int cursor = keyPosition + token.length();

  while (cursor < body.length() &&
         isJsonWhitespace(body[cursor])) {
    ++cursor;
  }

  if (cursor >= body.length() || body[cursor] != ':') {
    return false;
  }

  ++cursor;

  while (cursor < body.length() &&
         isJsonWhitespace(body[cursor])) {
    ++cursor;
  }

  if (cursor >= body.length() ||
      body[cursor] < '0' ||
      body[cursor] > '9') {
    return false;
  }

  uint64_t parsed = 0;

  while (cursor < body.length() &&
         body[cursor] >= '0' &&
         body[cursor] <= '9') {
    const uint8_t digit =
        static_cast<uint8_t>(body[cursor] - '0');

    const uint64_t next = parsed * 10ULL + digit;
    if (next < parsed) {
      return false;
    }

    parsed = next;
    ++cursor;
  }

  value = parsed;
  present = true;
  return true;
}

bool MarketDataService::validatePositivePrice(
    const char* price) const {
  if (price == nullptr || price[0] == '\0') {
    return false;
  }

  char* end = nullptr;
  const double parsed = strtod(price, &end);

  if (end == price || end == nullptr || *end != '\0') {
    return false;
  }

  return std::isfinite(parsed) && parsed > 0.0;
}

void MarketDataService::setError(const char* message) {
  if (message == nullptr) {
    _lastError[0] = '\0';
    return;
  }

  strncpy(_lastError, message, sizeof(_lastError) - 1);
  _lastError[sizeof(_lastError) - 1] = '\0';
}

void MarketDataService::setError(const String& message) {
  setError(message.c_str());
}
