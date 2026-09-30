#pragma once

#include "Bitmap1bpp.h"

struct DashboardState {
  const char* time;
  const Bitmap1bpp* weatherIcon;
  const char* weatherLabel;
  const char* temperature;
  const Bitmap1bpp* wifiIcon;
  const char* rssi;
  const char* btcPrice;
  const char* ethPrice;
  const char* hypePrice;
  const char* status;
};
