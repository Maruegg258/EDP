#include "MarketDataService.h"

#include <cmath>
#include <cstdlib>
#include <cstring>

namespace {

constexpr char BINANCE_FUTURES_TICKER_PREFIX[] =
    "https://fapi.binance.com/fapi/v2/ticker/price?symbol=";

constexpr char TRACKED_SYMBOLS[MarketDataService::TRACKED_SYMBOL_COUNT][
    MarketPriceValue::SYMBOL_CAPACITY] = {
  "BTCUSDT",
  "ETHUSDT",
  "HYPEUSDT"
};

bool isJsonWhitespace(char value) {
  return value == ' ' ||
         value == '\t' ||
         value == '\r' ||
         value == '\n';
}

bool isValidMarketSymbol(const char* symbol) {
  if (symbol == nullptr || symbol[0] == '\0') {
    return false;
  }

  for (const char* cursor = symbol; *cursor != '\0'; ++cursor) {
    const char value = *cursor;
    const bool uppercase = value >= 'A' && value <= 'Z';
    const bool digit = value >= '0' && value <= '9';

    if (!uppercase && !digit) {
      return false;
    }
  }

  return true;
}

}  // namespace

MarketDataService::MarketDataService(SecureHttpClient& httpClient)
    : _httpClient(httpClient) {
  memset(_slots, 0, sizeof(_slots));
  _requestError[0] = '\0';

  for (size_t index = 0;
       index < TRACKED_SYMBOL_COUNT;
       ++index) {
    _slots[index].symbol = TRACKED_SYMBOLS[index];
    _slots[index].lastError[0] = '\0';
  }
}

bool MarketDataService::fetchLatest(const char* symbol) {
  _requestError[0] = '\0';

  if (symbol == nullptr || symbol[0] == '\0') {
    setRequestError("market symbol is empty");
    return false;
  }

  if (strlen(symbol) >= MarketPriceValue::SYMBOL_CAPACITY) {
    setRequestError("market symbol exceeds service capacity");
    return false;
  }

  if (!isValidMarketSymbol(symbol)) {
    setRequestError("market symbol contains unsupported characters");
    return false;
  }

  const int slotIndex = findSlotIndex(symbol);

  if (slotIndex < 0) {
    setRequestError("market symbol is not configured");
    return false;
  }

  const size_t index = static_cast<size_t>(slotIndex);
  _slots[index].lastError[0] = '\0';

  String url(BINANCE_FUTURES_TICKER_PREFIX);
  url += symbol;

  SecureHttpResponse response;

  if (!_httpClient.get(url.c_str(), response)) {
    String error("HTTPS fetch failed");

    if (response.error.length() > 0) {
      error += ": ";
      error += response.error;
    }

    setRequestError(error);
    setSlotError(index, error);
    return false;
  }

  MarketPriceValue parsed = {};

  if (!parseTickerPayload(response.body, symbol, parsed)) {
    setSlotError(index, _requestError);
    return false;
  }

  _slots[index].lastValid = parsed;
  _slots[index].hasValidValue = true;
  _slots[index].lastError[0] = '\0';
  return true;
}

size_t MarketDataService::trackedSymbolCount() const {
  return TRACKED_SYMBOL_COUNT;
}

const char* MarketDataService::trackedSymbol(size_t index) const {
  if (index >= TRACKED_SYMBOL_COUNT) {
    return nullptr;
  }

  return _slots[index].symbol;
}

bool MarketDataService::hasValidValue(const char* symbol) const {
  const int slotIndex = findSlotIndex(symbol);

  if (slotIndex < 0) {
    return false;
  }

  return _slots[slotIndex].hasValidValue;
}

const MarketPriceValue* MarketDataService::lastValidValue(
    const char* symbol) const {
  const int slotIndex = findSlotIndex(symbol);

  if (slotIndex < 0 ||
      !_slots[slotIndex].hasValidValue) {
    return nullptr;
  }

  return &_slots[slotIndex].lastValid;
}

const char* MarketDataService::lastError(const char* symbol) const {
  const int slotIndex = findSlotIndex(symbol);

  if (slotIndex < 0) {
    return _requestError;
  }

  return _slots[slotIndex].lastError;
}

int MarketDataService::findSlotIndex(const char* symbol) const {
  if (symbol == nullptr) {
    return -1;
  }

  for (size_t index = 0;
       index < TRACKED_SYMBOL_COUNT;
       ++index) {
    if (strcmp(_slots[index].symbol, symbol) == 0) {
      return static_cast<int>(index);
    }
  }

  return -1;
}

bool MarketDataService::parseTickerPayload(
    const String& body,
    const char* requestedSymbol,
    MarketPriceValue& parsed) {
  if (body.length() == 0) {
    setRequestError("ticker response body is empty");
    return false;
  }

  if (!extractJsonString(
          body,
          "symbol",
          parsed.symbol,
          sizeof(parsed.symbol))) {
    setRequestError("ticker response has invalid or missing symbol");
    return false;
  }

  if (strcmp(parsed.symbol, requestedSymbol) != 0) {
    setRequestError("ticker response symbol does not match request");
    return false;
  }

  if (!extractJsonString(
          body,
          "price",
          parsed.price,
          sizeof(parsed.price))) {
    setRequestError("ticker response has invalid or missing price");
    return false;
  }

  if (!validatePositivePrice(parsed.price)) {
    setRequestError("ticker price is not a finite positive number");
    return false;
  }

  if (!extractOptionalJsonUint64(
          body,
          "time",
          parsed.sourceTime,
          parsed.hasSourceTime)) {
    setRequestError("ticker response time field is invalid");
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

    if (parsed > (UINT64_MAX - digit) / 10ULL) {
      return false;
    }

    parsed = parsed * 10ULL + digit;
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

void MarketDataService::setRequestError(const char* message) {
  if (message == nullptr) {
    _requestError[0] = '\0';
    return;
  }

  strncpy(
      _requestError,
      message,
      sizeof(_requestError) - 1
  );
  _requestError[sizeof(_requestError) - 1] = '\0';
}

void MarketDataService::setRequestError(const String& message) {
  setRequestError(message.c_str());
}

void MarketDataService::setSlotError(
    size_t index,
    const char* message) {
  if (index >= TRACKED_SYMBOL_COUNT) {
    return;
  }

  if (message == nullptr) {
    _slots[index].lastError[0] = '\0';
    return;
  }

  strncpy(
      _slots[index].lastError,
      message,
      sizeof(_slots[index].lastError) - 1
  );
  _slots[index].lastError[
      sizeof(_slots[index].lastError) - 1
  ] = '\0';
}

void MarketDataService::setSlotError(
    size_t index,
    const String& message) {
  setSlotError(index, message.c_str());
}
