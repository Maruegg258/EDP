# Phase 8B-6 — Live Dashboard regression / reliability acceptance

**Prepared:** 2026-10-11  
**Reference repository:** `Maruegg258/EDP`  
**Hardware-accepted baseline:** branch `phase8a1-static-prototype` at commit `546c9de0697ebb656a2cbcda34bb5c08cb1165b1` (Phase 8B-5B user-reported normal on-panel operation).  
**Production `main` at start:** `0471c1e63c218ecad85375e671cee674476db7f9` (unchanged).  
**Status:** Code-path inspection PASS for documented architectural invariants. **ESP32 build, specific fault simulations, continuous physical update quality, observed true midnight rollover and detailed menu-path device checks remain pending.**

## Scope / non-goals

This stage is **regression verification, not new functionality**. Use the exact accepted 8B-5B firmware and the existing ignored local `config.h`. Avoid changing the E-paper driver, partial maintenance sequence, TLS roots, WeatherService, MarketDataService or clock poll rate just to provoke conditions. Never use `client.setInsecure()`, fake live weather or fabricated six-day predictions.

The current production-integration test feature is enabled with **only**:

```cpp
// #define EDP_PHASE8A1_PREVIEW 1
// #define EDP_PHASE8_FONT_TEST 1
// #define EDP_PHASE8_WEATHER_ICON_TEST 1
// #define EDP_PHASE8B2_CLOCK_TEST 1
// #define EDP_PHASE8B3_MARKET_TEST 1
// #define EDP_PHASE8B4_WEATHER_TEST 1
// #define EDP_PHASE8B5A_WIFI_TEST 1
#define EDP_PHASE8B5B_APP_TEST 1
```

Preserve real `WIFI_SSID`, `WIFI_PASSWORD`, `WEATHER_LATITUDE` and `WEATHER_LONGITUDE` in **git-ignored** `config.h`. If already running this exact firmware from the 8B-5B hardware PASS, **no new upload is necessary**. If the firmware has changed since that test, first identify the exact previous known-good commit and differences, rather than stacking fixes. Open Serial Monitor at **115200 baud** for diagnostic logs and record which test steps actually ran.

## Source-level audit — completed; not hardware claims

| Invariant reviewed | Source and audit outcome |
| --- | --- |
| Weather and market services never refresh panel | `runPhase5B3MarketPollIfDue()`, `runProductionWeatherPollIfDue()`, `stagePhase8LiveDashboardFromServices()` stage values. Application owns `flushPendingPartial()`, page transitions and display I/O: **source PASS** |
| NTP date/weekday dirty | `Phase8CalendarPresentation::fromLocalTm()` formats owned local HH:MM/date/weekday. `Phase8DashboardFrameCompare::equal()` compares all three: **source PASS** |
| Market no-change suppression | `Phase8MarketPresentation::formatPrice()` creates bounded display strings; visual equality compares *displayed rounded text*, not full raw source precision: **source PASS** |
| Weather day/night dirty | `Phase8WeatherPresentation::fromCurrent()` selects approved immutable bitmap by condition + `isDay`; `equal()` compares bitmap pointer and label/temperature: **source PASS** |
| Wi-Fi icon dirty | RSSI sampled every 60s or on link-state change; selected 32×24 bitmap compared, not raw dBm; uses existing Phase 4 grades: **source PASS** |
| Display commit timing | `Phase8DashboardVisualState::stage()` records pending; `commitApplicationDashboard()` runs only after successful partial/maintenance/page redraw. Framebuffer copy then commit: **source PASS** |
| Refresh sequence | `CrowEPD579::maintenanceRefresh()`: fast clear → physical white → reinitialize/previous RAM white → current RAM new frame → partial refresh + synchronize previous RAM. `restoreFrameStateForPartial()` restores both RAM planes and `displayPartialFrame()` completes/synchronizes. **No driver modification** |
| Navigation / sleep / clean | `NavigationController`, `DashboardActionMenu`, `SystemPages`, `enterStandbyDeepSleep()`, `runDisplayClean()`, `flushVisiblePageChangeIfNeeded()` remain used in normal app. **Source PASS; every physical action not independently reverified** |
| Forecast truth | Live renderer writes `6-DAY FORECAST UNAVAILABLE`; Phase 8A fixture-only illustrative daily forecast is not used: **source PASS** |
| Transport | `SecureHttpClient` uses `NetworkClientSecure::setCACert`; no `setInsecure()` introduced: **source PASS** |

### Physical test 8B-6A — Short functional regression (about 10–20 minutes)

Run **first**, report results before attempting more intrusive tests. If a step fails, capture photo/serial output and stop changing code until analyzed.

| ID | Action | Required PASS |
| --- | --- | --- |
| A1 | Boot or restart the already accepted 8B-5B firmware, wait for Wi-Fi and NTP | Correct DD MMM / full weekday / HH:MM, ETH/BTC/HYPE real USDⓈ-M perpetual, actual Open-Meteo current weather, Wi-Fi grade; six-day lower region says `6-DAY FORECAST UNAVAILABLE`. No old Phase 7 screen |
| A2 | Press UP and DOWN through Dashboard, Weather and Markets, return Home | Pages display their respective data; no truncated text, double-image or ghosts across dual-controller seam at x≈396 |
| A3 | On Dashboard press MENU, observe DETAIL; optionally leave it open for 1–2 minutes, then EXIT | Menu stays visible during background polling; EXIT restores **latest live** Dashboard rather than the old design, and does not paint over the menu unexpectedly |
| A4 | MENU → DOWN once to select **DISPLAY CLEAN** → MENU to execute; then observe **three subsequent minute updates** | Expected Serial `DISPLAY CLEAN PASS: maintenance path complete; PAGE DASHBOARD.`; screen returns to newest modern Dashboard. Three later partials remain crisp, no blurry 34px font or new ghosting |
| A5 | MENU → **STANDBY** (default selection) → MENU | Small moon appears; ESP32 enters deep sleep and Wi-Fi is turned off; screen stays fixed |
| A6 | Wake using MENU or EXIT, release button and wait for NTP/network | Firmware restarts, modern Dashboard and data return without stale menu/panel artifacts; no accidental second button event |

**A4 is the most important immediate E-paper hardware regression.** Prior real-device observations found that misused full/fast full refresh could blur the next partial. Here use only original DETAIL `DISPLAY CLEAN` and already verified sequence—do **not** add or manually call full refresh.

**Expected normal diagnostics:** `PASS: Phase 7D navigation/menu self-checks.`, `PASS: Phase 8B-5B owned state and presentation self-checks.`, `Phase 7D input event:`, `DISPLAY CLEAN PASS: maintenance path complete; PAGE DASHBOARD.`, `Clock minute staged:`, and `Live dashboard flush dirty:` when *visible* home state changed. Actual event order and values will vary.

### Physical test 8B-6B — Network interruption and displayed-state suppression (suggested 30–45 minutes)

| ID | Action | Required PASS |
| --- | --- | --- |
| B1 | If safe and convenient, temporarily disable only the board's Wi-Fi source for about 2–3 minutes | Wi-Fi disconnect icon appears; last valid BTC/ETH/HYPE and current weather remain visible; do not show API failure messages as price/forecast. With an already synchronized RTC, HH:MM should continue to advance |
| B2 | Restore Wi-Fi and wait for the next due fetch | Wi-Fi grade returns, subsequent public HTTPS calls succeed or retry on their normal cadences. Last valid data updates when actual values change; no hardcoded fallback forecasts |
| B3 | Watch RSSI and normal polls for a few minutes | If raw dBm changes without crossing the icon grade, it must not independently refresh the panel; a *normal minute clock change* **will** still refresh, so don't mistake that as a Wi-Fi-driven extra refresh |
| B4 | Leave device online across one full ~30-minute weather poll interval | `Production weather poll starting...` occurs at the scheduled interval; a poll with unchanged visible weather should not by itself trigger another Dashboard refresh |

Only test disconnect/reconnect if it will not disrupt other important devices. Do not modify network certificates, weather data or system clock to force faults. Mark steps **DEFERRED** if not performed.

### Physical test 8B-6C — E-paper endurance and date boundary (suggested 1–2 hours; overnight optional)

| ID | Action | Required PASS |
| --- | --- | --- |
| C1 | Leave Dashboard on for 1–2 hours with minute-level clock changes | No steadily accumulating shadow text, fuzzy 34px characters, unexpected full-screen flashing or lost partial updates; inspect the physical x≈396 seam and upper weather icon |
| C2 | After at least one maintenance cleanup, inspect next 3–5 partial refreshes | Strokes remain sharp and old characters disappear. Do not introduce periodic Full Refresh as a workaround |
| C3 | If convenient, leave running over a real Taiwan-local midnight | Date, HH:MM and English weekday all change correctly together on the next visible refresh; do not force the ESP32 clock to a fabricated value |
| C4 | If actual weather crosses sunrise/sunset while observed | For conditions with distinct day/night artwork, icon change should be detected even when temperature stays the same. Do not claim a test when the real data never changed |

The user is USB-C powered, so **battery current measurement is outside this gate**. Photos of legibility are optional evidence; serial logs plus direct visual findings are acceptable.

## Evidence / sign-off matrix

| Area | Current status | Physical acceptance evidence |
| --- | --- | --- |
| Previous 8B-5B normal live Dashboard | **PASS (user reported)** | Hardware result reported 2026-10-11 |
| 8B-6 source architecture/refresh audit | **PASS (source review only)** | GitHub branch inspected; no firmware modified |
| 8B-6A short navigation + clean + standby | **PENDING** | Request individual A1–A6 results |
| 8B-6B Wi-Fi + 30-minute weather poll | **PENDING** | Request B1–B4; optional tests may be deferred |
| 8B-6C long-run + real midnight/day-night | **PENDING** | Request C1–C4; mark unobserved events DEFERRED, never PASS |

**Do not close 8B-6 by assuming that every test is automatically covered by the earlier general 8B-5B report.** After each physical test report, record exactly which IDs passed/failed/deferred in this file and `ROADMAP.md`, with firmware/code changes only if evidence shows an actual defect. If a regression appears, start from the accepted 8B-5B HEAD `546c9de0697ebb656a2cbcda34bb5c08cb1165b1` and isolate the smallest reproducible difference before committing a fix.

## Next stage boundary

Phase 8C will introduce **real six-day daily weather**. Neither 8B-6 test scripts nor production UI should display the old illustrative `MON–SAT` temperatures/precipitation as actual forecasts. No new network listeners, OTA, BLE, MQTT, telemetry or privileged exchange API keys.
