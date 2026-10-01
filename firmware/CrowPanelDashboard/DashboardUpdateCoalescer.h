#pragma once

#include "DashboardDirty.h"
#include "DashboardState.h"
#include "DashboardStateSnapshot.h"
#include "WidgetStates.h"

class DashboardUpdateCoalescer {
public:
  DashboardUpdateCoalescer();

  bool establishDisplayed(const DashboardState& state);

  bool stage(const DashboardState& state);
  bool stageClock(const ClockWidgetState& state);
  bool stageWeather(const WeatherWidgetState& state);
  bool stageWiFi(const WiFiWidgetState& state);
  bool stageBtc(const CryptoWidgetState& state);
  bool stageEth(const CryptoWidgetState& state);
  bool stageHype(const CryptoWidgetState& state);
  bool stageStatus(const StatusWidgetState& state);

  bool hasDisplayedState() const;
  bool hasPendingUpdate() const;
  DashboardDirtyMask pendingDirty() const;

  const DashboardState& displayedState() const;
  const DashboardState& pendingState() const;

  bool commitPending();
  void discardPending();

private:
  bool ensurePendingBase();
  bool captureMerged(const DashboardState& state);
  void refreshPendingDirty();

  DashboardStateSnapshot _displayed;
  DashboardStateSnapshot _pending;
  bool _hasPending;
  DashboardDirtyMask _pendingDirty;
};
