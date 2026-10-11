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
// Use Arduino Serial Monitor at 115200 baud, send 1/2/3/4/5 to change pages.
// 1:34px, 2:17px, 3:13px, 4:14px, 5:13px-vs-14px.
// This runs without Wi-Fi, live data, navigation or timed refresh.
// Do NOT enable simultaneously with EDP_PHASE8A1_PREVIEW.
// #define EDP_PHASE8_FONT_TEST 1

// Phase 8A-2 11-item weather icon gallery (feature branch only).
// In LOCAL ignored config.h, uncomment to view one static 792x272 test page.
// Must disable both EDP_PHASE8_FONT_TEST and EDP_PHASE8A1_PREVIEW.
// No Wi-Fi, HTTP, polling, buttons or automatic refresh in this mode.
// #define EDP_PHASE8_WEATHER_ICON_TEST 1

// Phase 8B-2 isolated REAL NTP clock/calendar test (feature branch only).
// This is NOT production live Dashboard: crypto, current weather, forecast
// and Wi-Fi icon remain STATIC MOCKS even though Wi-Fi is used for NTP.
// Exactly ONE of the four preview/test flags may be enabled.
// Shows WAITING while no valid NTP time, then HH:MM / DD MMM / WEEKDAY.
// Only changed calendar text triggers the existing partial-refresh sequence.
// Requires real WIFI_SSID and WIFI_PASSWORD in the ignored local config.h.
// #define EDP_PHASE8B2_CLOCK_TEST 1

// Phase 8B-3 isolated LIVE NTP + Binance USDT-M perpetual market test.
// Exactly ONE preview/test flag may be 1. Keep WIFI credentials local.
// Weather (SUNNY/26.9 C), six-day forecast and drawn Wi-Fi icon are MOCK;
// the NTP calendar and three ETH/BTC/HYPE prices are actually LIVE.
// Prices are rounded only for 34px display, never modified in service data.
// The three symbols are polled together every 60s using existing verified TLS.
// #define EDP_PHASE8B3_MARKET_TEST 1

// Phase 8B-4 isolated live NTP + BTC/ETH/HYPE + Open-Meteo CURRENT weather.
// All previous preview/test flags must be disabled. Retain working local
// WIFI_SSID, WIFI_PASSWORD, WEATHER_LATITUDE, WEATHER_LONGITUDE.
// Weather current condition, 1-decimal Celsius and day/night icon are LIVE.
// Bottom 6-day forecast and top-right Wi-Fi strength graphic remain MOCK.
// Market polls every 60s, weather every 30min; one application-owned partial
// refresh for visible changes, verified HTTPS roots, no new network service.
// #define EDP_PHASE8B4_WEATHER_TEST 1
