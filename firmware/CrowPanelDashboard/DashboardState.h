#pragma once

#include "WidgetStates.h"

struct DashboardState {
  ClockWidgetState clock;
  WeatherWidgetState weather;
  WiFiWidgetState wifi;
  CryptoWidgetState btc;
  CryptoWidgetState eth;
  CryptoWidgetState hype;
  StatusWidgetState status;
};
