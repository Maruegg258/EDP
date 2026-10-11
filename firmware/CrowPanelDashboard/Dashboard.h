#pragma once

#include "ClockWidget.h"
#include "CryptoWidget.h"
#include "DashboardState.h"
#include "Phase8DashboardFrame.h"
#include "GraphicsBW.h"
#include "StatusWidget.h"
#include "WeatherWidget.h"
#include "WiFiWidget.h"

class Dashboard {
public:
  explicit Dashboard(GraphicsBW& graphics);

  bool render(const DashboardState& state);
  // 8B-5B: approved modern UI; no polling, page actions or E-paper I/O.
  bool renderLive(const Phase8DashboardFrame& frame);

private:
  GraphicsBW& _graphics;
  WeatherWidget _weather;
  ClockWidget _clock;
  WiFiWidget _wifi;
  CryptoWidget _btc;
  CryptoWidget _eth;
  CryptoWidget _hype;
  StatusWidget _status;
};
