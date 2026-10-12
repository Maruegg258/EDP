#pragma once

// Copy this file to config.h and fill in local values.
// Never commit config.h.

#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

// Optional PEAP (802.1X) login, ONLY in ignored LOCAL config.h.
// Requires permission to attach a personal device to this network.
// #define WIFI_USE_ENTERPRISE 1
// #define WIFI_EAP_IDENTITY "YOUR_OUTER_IDENTITY"
// #define WIFI_EAP_USERNAME "YOUR_USERNAME"
// #define WIFI_EAP_PASSWORD "YOUR_ENTERPRISE_PASSWORD"
// #define WIFI_EAP_SERVER_NAME "EXPECTED_RADIUS_CERT_HOSTNAME"
//
// In LOCAL config.h, additionally declare the verified PUBLIC CA PEM as:
// static const char WIFI_EAP_CA_PEM[] = R"EDPPEM(-----BEGIN CERTIFICATE-----
// ...base64 content from verified trusted CA...
// -----END CERTIFICATE-----
// )EDPPEM";
// No server leaf certificate pinning; no missing-CA fallback.
// See docs/ENTERPRISE_WIFI.md before enabling.

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

// Phase 8B-5A (FIRST GATE of Phase 8B-5):
// Opt-in live NTP, BTC/ETH/HYPE, current Open-Meteo weather and Wi-Fi RSSI.
// Requires ignored local Wi-Fi credentials AND weather latitude/longitude.
// All other EDP_PHASE8* preview/test flags must be commented out.
// RSSI mapping: >=-60 strong; >=-70 medium; >=-80 weak; else very weak;
// disconnected/connecting shows X. Sample 60s plus immediate state changes.
// Still an ISOLATED experimental page. No buttons/navigation in this mode;
// six-day forecast remains illustrative MOCK data. Phase 8B-5B production
// Dashboard/Dirty/PageRenderer integration is deliberately NOT included.
// #define EDP_PHASE8B5A_WIFI_TEST 1

// Phase 8B-5B: opt-in NORMAL Phase 7 application with the visually accepted
// modern Phase 8 dashboard. Unlike the 8B-5A isolated test this re-enables
// UP/DOWN page navigation, DETAIL MENU, DISPLAY CLEAN, standby / deep sleep.
// The six daily forecast fixtures are replaced by "6-DAY FORECAST UNAVAILABLE"
// until real six-day daily data is integrated in Phase 8C. Existing TLS,
// service polling and the verified maintenance/partial refresh are reused.
// Requires local WIFI credentials and WEATHER_LATITUDE/LONGITUDE.
// Turn ALL previous EDP_PHASE8* test/preview flags off before enabling.
// #define EDP_PHASE8B5B_APP_TEST 1

// Phase 8C-1: serial-only LIVE Open-Meteo 7-date test (today + next six).
// Disable ALL EDP_PHASE8* older flags; keep your Wi-Fi and weather latitude /
// longitude ONLY in local ignored config.h. Serial Monitor 115200 baud.
// Prints date, WMO weather condition, max/min Celsius, precipitation max for
// six future days after HTTPS certificate validation. Does NOT draw or refresh
// the E-paper, poll Binance, or enable normal app/page navigation.
// No fake forecast. Next 8C-2 will map these values into the 6 visual columns.
// #define EDP_PHASE8C1_DAILY_TEST 1

// Phase 8C-2: isolated ONE-SHOT rendering of SIX REAL future-day columns.
// Disable all EDP_PHASE8* other modes; preserve local WIFI credentials and
// WEATHER_LATITUDE/LONGITUDE in ignored config.h. This fetches today's
// current weather + 6 real future days using the existing verified TLS.
// The upper time/price remains WAITING/-- (NTP/Binance disabled), so only
// lower weather typography/icons are under test. MaintenanceRefresh once.
// Does NOT enter normal Phase 7 navigation, poll repeatedly, or integrate
// Phase 8C-3 dirty tracking; earlier preview modes are unchanged.
// Serial Monitor: 115200 baud.
// #define EDP_PHASE8C2_DISPLAY_TEST 1

// Phase 8C-3: FULL modern Dashboard (Phase 8B-5B app) with six REAL
// future-day Open-Meteo forecasts and existing 30-minute service cadence.
// Set EDP_PHASE8B5B_APP_TEST=1 AND EDP_PHASE8C3_APP_TEST=1 in ignored
// local config.h, with all isolated preview/test flags commented out.
// Both flags are required: this is normal Phase 7 page/DETAIL/deep-sleep app.
// WeatherService::fetchLatestSixDay reuses TLS-validating SecureHttpClient.
// No independent polling or refresh timer. Rendered forecast is compared in
// Phase8DashboardVisualState and committed only after a successful panel draw.
// If daily values are unavailable or won't fit, display UNAVAILABLE or retain
// prior valid forecast. Full endurance is deferred to final Phase 8 sign-off.
// #define EDP_PHASE8C3_APP_TEST 1
