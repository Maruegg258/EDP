#pragma once

#include "Phase8CalendarPresentation.h"
#include "Phase8MarketPresentation.h"
#include "Phase8WeatherPresentation.h"

// Phase 8B-5B, fixed-size, owned, visible-only Dashboard presentation.
// No services, network, E-paper driver or temporary string pointers.
struct Phase8DashboardFrame {
  Phase8CalendarHeader calendar;
  Phase8MarketPrices markets;
  Phase8CurrentWeather weather;
  const Bitmap1bpp* wifi;
};

namespace Phase8DashboardFrameCompare {
void setUnavailable(Phase8DashboardFrame& frame);
bool equal(const Phase8DashboardFrame& a, const Phase8DashboardFrame& b);
bool selfCheck();
}

// The application stages a full candidate after polling and commits only
// AFTER a successful physical E-paper update (including page transitions).
class Phase8DashboardVisualState {
public:
  Phase8DashboardVisualState();
  const Phase8DashboardFrame& displayed() const { return _displayed; }
  const Phase8DashboardFrame& latest() const {
    return _hasPending ? _pending : _displayed;
  }
  bool hasDisplayed() const { return _hasDisplayed; }
  bool hasPending() const { return _hasPending; }
  void establishDisplayed(const Phase8DashboardFrame& frame);
  bool stage(const Phase8DashboardFrame& frame);
  bool commitPending();
private:
  Phase8DashboardFrame _displayed;
  Phase8DashboardFrame _pending;
  bool _hasDisplayed;
  bool _hasPending;
};
