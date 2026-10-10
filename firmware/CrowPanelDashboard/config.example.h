#pragma once

// Copy this file to config.h and fill in local values.
// Never commit config.h.

#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

// Phase 6 weather location.
// Keep your real coordinates only in local config.h.
// Uncomment and replace both values with decimal-degree numbers.
// #define WEATHER_LATITUDE  YOUR_LATITUDE_AS_DECIMAL
// #define WEATHER_LONGITUDE YOUR_LONGITUDE_AS_DECIMAL

// Phase 8A-1 preview only (feature branch):
// Uncomment in LOCAL config.h to render one static dashboard frame at boot.
// The preview intentionally does not connect Wi-Fi or poll weather/markets.
// Remove/comment it after testing to resume normal production firmware.
// #define EDP_PHASE8A1_PREVIEW 1
