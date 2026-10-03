#pragma once

#include "Bitmap1bpp.h"

struct ClockWidgetState {
  const char* time;
};

struct WeatherWidgetState {
  const Bitmap1bpp* icon;
  const char* label;
  const char* temperature;
};

struct WiFiWidgetState {
  const Bitmap1bpp* icon;
};

struct CryptoWidgetState {
  const char* price;
};

struct StatusWidgetState {
  const char* text;
};
