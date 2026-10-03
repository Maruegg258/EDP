#pragma once

#include <stddef.h>

#include "DashboardState.h"

class DashboardStateSnapshot {
public:
  DashboardStateSnapshot();

  DashboardStateSnapshot(const DashboardStateSnapshot&) = delete;
  DashboardStateSnapshot& operator=(const DashboardStateSnapshot&) = delete;

  bool capture(const DashboardState& source);

  bool valid() const;
  const DashboardState& state() const;

private:
  static bool fitsText(const char* source, size_t capacity);
  static void copyText(char* destination,
                       size_t capacity,
                       const char* source);

  bool _valid;

  char _time[8];
  char _weatherLabel[16];
  char _temperature[16];
  char _btcPrice[24];
  char _ethPrice[24];
  char _hypePrice[24];
  char _status[48];

  DashboardState _state;
};
