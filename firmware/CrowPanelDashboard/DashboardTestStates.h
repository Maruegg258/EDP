#pragma once

#include "DashboardState.h"
#include "Icons.h"

namespace DashboardTestStates {

static const DashboardState BASELINE = {
  { "12:34" },
  { &Icons::WEATHER_SUN, "SUN", "28 C" },
  { &Icons::WIFI_STRONG },
  { "65234.50" },
  { "3921.75" },
  { "48.26" },
  { "FULL BASELINE" }
};

static const DashboardState PRE_MAINTENANCE = {
  { "12:35" },
  { &Icons::WEATHER_CLOUD, "CLOUD", "27 C" },
  { &Icons::WIFI_MEDIUM },
  { "65240.10" },
  { "3924.20" },
  { "48.40" },
  { "PARTIAL ONE" }
};

static const DashboardState MAINTENANCE = {
  { "12:36" },
  { &Icons::WEATHER_RAIN, "RAIN", "26 C" },
  { &Icons::WIFI_WEAK },
  { "65210.25" },
  { "3918.50" },
  { "48.05" },
  { "MAINTENANCE" }
};

static const DashboardState POST_MAINTENANCE[] = {
  {
    { "12:37" },
    { &Icons::WEATHER_CLOUD, "CLOUD", "26 C" },
    { &Icons::WIFI_MEDIUM },
    { "65218.80" },
    { "3920.10" },
    { "48.12" },
    { "POST PARTIAL ONE" }
  },
  {
    { "12:38" },
    { &Icons::WEATHER_SUN, "SUN", "27 C" },
    { &Icons::WIFI_STRONG },
    { "65255.60" },
    { "3928.40" },
    { "48.55" },
    { "POST PARTIAL TWO" }
  },
  {
    { "12:39" },
    { &Icons::WEATHER_RAIN, "RAIN", "25 C" },
    { &Icons::WIFI_DISCONNECTED },
    { "65205.15" },
    { "3915.25" },
    { "47.98" },
    { "POST PARTIAL THREE" }
  }
};

}  // namespace DashboardTestStates
