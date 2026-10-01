#pragma once

#include "WidgetStates.h"

namespace WidgetStateCompare {

bool changed(const ClockWidgetState& previous,
             const ClockWidgetState& current);

bool changed(const WeatherWidgetState& previous,
             const WeatherWidgetState& current);

bool changed(const WiFiWidgetState& previous,
             const WiFiWidgetState& current);

bool changed(const CryptoWidgetState& previous,
             const CryptoWidgetState& current);

bool changed(const StatusWidgetState& previous,
             const StatusWidgetState& current);

}  // namespace WidgetStateCompare
