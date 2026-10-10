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

// Rev-C3 font-only diagnostic mode (feature branch only).
// Add to LOCAL ignored config.h and upload once. On boot, font page 1 displays.
// Use Arduino Serial Monitor at 115200 baud, send 1/2/3/4 to change pages.
// This runs without Wi-Fi, live data, navigation or timed refresh.
// Do NOT enable simultaneously with EDP_PHASE8A1_PREVIEW.
// #define EDP_PHASE8_FONT_TEST 1
