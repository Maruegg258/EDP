#include "DashboardStateCompare.h"

#include "WidgetStateCompare.h"

DashboardDirtyMask detectDashboardDirty(
    const DashboardState& previous,
    const DashboardState& current) {
  DashboardDirtyMask dirty = DashboardDirty::NONE;

  if (WidgetStateCompare::changed(previous.clock, current.clock)) {
    dirty |= DashboardDirty::CLOCK;
  }

  if (WidgetStateCompare::changed(previous.weather, current.weather)) {
    dirty |= DashboardDirty::WEATHER;
  }

  if (WidgetStateCompare::changed(previous.wifi, current.wifi)) {
    dirty |= DashboardDirty::WIFI;
  }

  if (WidgetStateCompare::changed(previous.btc, current.btc)) {
    dirty |= DashboardDirty::BTC;
  }

  if (WidgetStateCompare::changed(previous.eth, current.eth)) {
    dirty |= DashboardDirty::ETH;
  }

  if (WidgetStateCompare::changed(previous.hype, current.hype)) {
    dirty |= DashboardDirty::HYPE;
  }

  if (WidgetStateCompare::changed(previous.status, current.status)) {
    dirty |= DashboardDirty::STATUS;
  }

  return dirty;
}
