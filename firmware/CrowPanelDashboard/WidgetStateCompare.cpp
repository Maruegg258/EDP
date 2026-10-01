#include "WidgetStateCompare.h"

#include <cstring>

namespace {

bool sameText(const char* left, const char* right) {
  if (left == right) {
    return true;
  }

  if (left == nullptr || right == nullptr) {
    return false;
  }

  return strcmp(left, right) == 0;
}

}  // namespace

namespace WidgetStateCompare {

bool changed(const ClockWidgetState& previous,
             const ClockWidgetState& current) {
  return !sameText(previous.time, current.time);
}

bool changed(const WeatherWidgetState& previous,
             const WeatherWidgetState& current) {
  return previous.icon != current.icon ||
         !sameText(previous.label, current.label) ||
         !sameText(previous.temperature, current.temperature);
}

bool changed(const WiFiWidgetState& previous,
             const WiFiWidgetState& current) {
  return previous.icon != current.icon ||
         !sameText(previous.rssi, current.rssi);
}

bool changed(const CryptoWidgetState& previous,
             const CryptoWidgetState& current) {
  return !sameText(previous.price, current.price);
}

bool changed(const StatusWidgetState& previous,
             const StatusWidgetState& current) {
  return !sameText(previous.text, current.text);
}

}  // namespace WidgetStateCompare
