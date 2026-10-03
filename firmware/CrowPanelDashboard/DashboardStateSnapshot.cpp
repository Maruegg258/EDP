#include "DashboardStateSnapshot.h"

#include <cstring>

DashboardStateSnapshot::DashboardStateSnapshot()
    : _valid(false),
      _state{} {
  _time[0] = '\0';
  _weatherLabel[0] = '\0';
  _temperature[0] = '\0';
  _btcPrice[0] = '\0';
  _ethPrice[0] = '\0';
  _hypePrice[0] = '\0';
  _status[0] = '\0';

  _state.clock.time = _time;

  _state.weather.icon = nullptr;
  _state.weather.label = _weatherLabel;
  _state.weather.temperature = _temperature;

  _state.wifi.icon = nullptr;

  _state.btc.price = _btcPrice;
  _state.eth.price = _ethPrice;
  _state.hype.price = _hypePrice;

  _state.status.text = _status;
}

bool DashboardStateSnapshot::fitsText(const char* source,
                                      size_t capacity) {
  return source != nullptr &&
         strlen(source) < capacity;
}

void DashboardStateSnapshot::copyText(char* destination,
                                      size_t capacity,
                                      const char* source) {
  const size_t length = strlen(source);
  const size_t copyLength =
      length < (capacity - 1) ? length : (capacity - 1);

  memcpy(destination, source, copyLength);
  destination[copyLength] = '\0';
}

bool DashboardStateSnapshot::capture(const DashboardState& source) {
  if (source.weather.icon == nullptr ||
      source.wifi.icon == nullptr ||
      !fitsText(source.clock.time, sizeof(_time)) ||
      !fitsText(source.weather.label, sizeof(_weatherLabel)) ||
      !fitsText(source.weather.temperature, sizeof(_temperature)) ||
      !fitsText(source.btc.price, sizeof(_btcPrice)) ||
      !fitsText(source.eth.price, sizeof(_ethPrice)) ||
      !fitsText(source.hype.price, sizeof(_hypePrice)) ||
      !fitsText(source.status.text, sizeof(_status))) {
    return false;
  }

  copyText(_time, sizeof(_time), source.clock.time);
  copyText(
      _weatherLabel,
      sizeof(_weatherLabel),
      source.weather.label);
  copyText(
      _temperature,
      sizeof(_temperature),
      source.weather.temperature);
  copyText(_btcPrice, sizeof(_btcPrice), source.btc.price);
  copyText(_ethPrice, sizeof(_ethPrice), source.eth.price);
  copyText(_hypePrice, sizeof(_hypePrice), source.hype.price);
  copyText(_status, sizeof(_status), source.status.text);

  _state.weather.icon = source.weather.icon;
  _state.wifi.icon = source.wifi.icon;

  _valid = true;
  return true;
}

bool DashboardStateSnapshot::valid() const {
  return _valid;
}

const DashboardState& DashboardStateSnapshot::state() const {
  return _state;
}
