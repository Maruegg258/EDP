#include "DashboardUpdateCoalescer.h"

#include "DashboardStateCompare.h"

DashboardUpdateCoalescer::DashboardUpdateCoalescer()
    : _hasPending(false),
      _pendingDirty(DashboardDirty::NONE) {
}

bool DashboardUpdateCoalescer::establishDisplayed(
    const DashboardState& state) {
  if (!_displayed.capture(state)) {
    return false;
  }

  _hasPending = false;
  _pendingDirty = DashboardDirty::NONE;
  return true;
}

bool DashboardUpdateCoalescer::ensurePendingBase() {
  if (!_displayed.valid()) {
    return false;
  }

  if (_hasPending) {
    return true;
  }

  return _pending.capture(_displayed.state());
}

bool DashboardUpdateCoalescer::captureMerged(
    const DashboardState& state) {
  if (!_pending.capture(state)) {
    return false;
  }

  refreshPendingDirty();
  return true;
}

void DashboardUpdateCoalescer::refreshPendingDirty() {
  _pendingDirty = detectDashboardDirty(
      _displayed.state(),
      _pending.state()
  );
  _hasPending = _pendingDirty != DashboardDirty::NONE;
}

bool DashboardUpdateCoalescer::stage(
    const DashboardState& state) {
  if (!_displayed.valid()) {
    return false;
  }

  return captureMerged(state);
}

bool DashboardUpdateCoalescer::stageClock(
    const ClockWidgetState& state) {
  if (!ensurePendingBase()) {
    return false;
  }

  DashboardState merged = _pending.state();
  merged.clock = state;
  return captureMerged(merged);
}

bool DashboardUpdateCoalescer::stageWeather(
    const WeatherWidgetState& state) {
  if (!ensurePendingBase()) {
    return false;
  }

  DashboardState merged = _pending.state();
  merged.weather = state;
  return captureMerged(merged);
}

bool DashboardUpdateCoalescer::stageWiFi(
    const WiFiWidgetState& state) {
  if (!ensurePendingBase()) {
    return false;
  }

  DashboardState merged = _pending.state();
  merged.wifi = state;
  return captureMerged(merged);
}

bool DashboardUpdateCoalescer::stageBtc(
    const CryptoWidgetState& state) {
  if (!ensurePendingBase()) {
    return false;
  }

  DashboardState merged = _pending.state();
  merged.btc = state;
  return captureMerged(merged);
}

bool DashboardUpdateCoalescer::stageEth(
    const CryptoWidgetState& state) {
  if (!ensurePendingBase()) {
    return false;
  }

  DashboardState merged = _pending.state();
  merged.eth = state;
  return captureMerged(merged);
}

bool DashboardUpdateCoalescer::stageHype(
    const CryptoWidgetState& state) {
  if (!ensurePendingBase()) {
    return false;
  }

  DashboardState merged = _pending.state();
  merged.hype = state;
  return captureMerged(merged);
}

bool DashboardUpdateCoalescer::stageStatus(
    const StatusWidgetState& state) {
  if (!ensurePendingBase()) {
    return false;
  }

  DashboardState merged = _pending.state();
  merged.status = state;
  return captureMerged(merged);
}

bool DashboardUpdateCoalescer::hasDisplayedState() const {
  return _displayed.valid();
}

bool DashboardUpdateCoalescer::hasPendingUpdate() const {
  return _hasPending;
}

DashboardDirtyMask DashboardUpdateCoalescer::pendingDirty() const {
  return _pendingDirty;
}

const DashboardState& DashboardUpdateCoalescer::displayedState() const {
  return _displayed.state();
}

const DashboardState& DashboardUpdateCoalescer::pendingState() const {
  return _pending.state();
}

bool DashboardUpdateCoalescer::commitPending() {
  if (!_hasPending) {
    return true;
  }

  if (!_displayed.capture(_pending.state())) {
    return false;
  }

  _hasPending = false;
  _pendingDirty = DashboardDirty::NONE;
  return true;
}

void DashboardUpdateCoalescer::discardPending() {
  _hasPending = false;
  _pendingDirty = DashboardDirty::NONE;
}
