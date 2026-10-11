#include "Phase8DashboardFrame.h"

#include <cstring>

#include "Icons.h"

namespace Phase8DashboardFrameCompare {

void setUnavailable(Phase8DashboardFrame& frame) {
  strcpy(frame.calendar.time, "--:--");
  strcpy(frame.calendar.date, "-- ---");
  strcpy(frame.calendar.weekday, "WAITING");
  Phase8MarketPresentation::setUnavailable(frame.markets);
  Phase8WeatherPresentation::setUnavailable(frame.weather);
  frame.wifi = &Icons::WIFI_DISCONNECTED;
}

bool equal(const Phase8DashboardFrame& a, const Phase8DashboardFrame& b) {
  return Phase8CalendarPresentation::equal(a.calendar, b.calendar) &&
         Phase8MarketPresentation::equal(a.markets, b.markets) &&
         Phase8WeatherPresentation::equal(a.weather, b.weather) &&
         a.wifi == b.wifi;
}

bool selfCheck() {
  Phase8DashboardFrame initial{};
  setUnavailable(initial);
  Phase8DashboardVisualState state;
  if (state.hasDisplayed() || state.hasPending() ||
      !equal(initial, state.displayed())) return false;

  state.establishDisplayed(initial);
  if (!state.hasDisplayed() || !state.stage(initial) ||
      state.hasPending()) return false;

  Phase8DashboardFrame update = initial;
  strcpy(update.calendar.date, "12 OCT");
  if (!state.stage(update) || !state.hasPending() ||
      strcmp(state.latest().calendar.date, "12 OCT") != 0 ||
      strcmp(state.displayed().calendar.date, "-- ---") != 0) return false;
  if (!state.commitPending() || state.hasPending() ||
      strcmp(state.displayed().calendar.date, "12 OCT") != 0) return false;

  update.wifi = &Icons::WIFI_STRONG;
  if (!state.stage(update) || !state.hasPending()) return false;
  // Simulate pending + return to displayed, or a failed refresh:
  // no committed bitmap changes until success.
  if (state.displayed().wifi != &Icons::WIFI_DISCONNECTED) return false;
  if (!state.stage(state.displayed()) || state.hasPending()) return false;

  Phase8CurrentWeather clear{};
  WeatherCurrentValue weather{};
  weather.condition = WeatherCondition::CLEAR;
  weather.isDay = true;
  weather.temperatureC = 22.0f;
  if (!Phase8WeatherPresentation::fromCurrent(weather, clear)) return false;
  update = initial;
  update.weather = clear;
  state.establishDisplayed(initial);
  if (!state.stage(update) || !state.hasPending()) return false;

  weather.isDay = false;
  Phase8CurrentWeather night{};
  if (!Phase8WeatherPresentation::fromCurrent(weather, night)) return false;
  update.weather = night;
  if (!state.stage(update) || !state.hasPending() ||
      state.latest().weather.icon == clear.icon) return false;
  return state.commitPending() &&
         state.displayed().weather.icon == night.icon;
}

}  // namespace Phase8DashboardFrameCompare

Phase8DashboardVisualState::Phase8DashboardVisualState()
    : _displayed{}, _pending{},
      _hasDisplayed(false), _hasPending(false) {
  Phase8DashboardFrameCompare::setUnavailable(_displayed);
  _pending = _displayed;
}

void Phase8DashboardVisualState::establishDisplayed(
    const Phase8DashboardFrame& frame) {
  _displayed = frame;
  _pending = frame;
  _hasDisplayed = true;
  _hasPending = false;
}

bool Phase8DashboardVisualState::stage(
    const Phase8DashboardFrame& frame) {
  if (!_hasDisplayed || frame.weather.icon == nullptr ||
      frame.wifi == nullptr) return false;
  _pending = frame;
  _hasPending = !Phase8DashboardFrameCompare::equal(_displayed, _pending);
  return true;
}

bool Phase8DashboardVisualState::commitPending() {
  if (!_hasDisplayed) return false;
  if (_hasPending) _displayed = _pending;
  _hasPending = false;
  return true;
}
