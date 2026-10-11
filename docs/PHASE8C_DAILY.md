# Phase 8C-1 — Real six-future-day Open-Meteo data contract

**Date:** 2026-10-11  
**Feature branch:** `phase8a1-static-prototype`  
**Current status:** Source implementation and GitHub readback/checks complete. **User reports the Phase 8C-1 serial-only real-hardware test operated normally (2026-10-11); 8C-1 data-acquisition checkpoint ACCEPTED.** Detailed raw TLS/Serial transcript was not supplied. This step performs **NO E-paper refresh or Dashboard forecast rendering.**

## Boundary agreed after Phase 8B-6B

The user requested that the previously proposed Phase 8B-6C **1–2 hour / overnight E-paper endurance** be moved to the **very end of Phase 8**, after every Dashboard feature is implemented. Roadmap now records this as **deferred, not PASS**. Phase 8B-6A and 8B-6B accepted hardware checks remain valid. Do not repeat long-term tests on an incomplete Dashboard unless a concrete defect appears.

## Provider data contract

Keep the existing **TLS-certificate-validated** `WeatherService` and `SecureHttpClient` network stack. Open-Meteo's public `/v1/forecast` permits `forecast_days=7` and `daily` variables `weather_code`, `temperature_2m_max`, `temperature_2m_min`, `precipitation_probability_max`. Use the existing `timezone=Asia%2FTaipei` and current/hourly request fields.

Seven daily dates mean:

- `daily[0]`: **today**, aligned to `current.time` date (not shown in the six columns).
- `daily[1]` through `daily[6]`: **tomorrow through six days from today**, exactly six prospective forecast columns.

The existing production `WeatherService::fetchLatest(latitude,longitude)` and its **two-day** `forecast_days=2` URL are unchanged. New explicitly opt-in `fetchLatestSixDay(latitude,longitude)` requests `forecast_days=7`, reusing the same HTTP client, parser helpers, WMO normalization and atomic snapshot update. This is not a second provider implementation.

`WeatherSnapshot` retains `today`, `tomorrow`, the five existing hourly future slots, and current weather. It now includes `futureDays[6]` and `hasSixDayForecast` (true only for a validated seven-day response); for the new mode, `futureDays[0]` is the same data as `tomorrow`. **Do not read `futureDays` as a complete six-day forecast unless `hasSixDayForecast` is true.**

### Strict validation and last-valid preservation

- All `daily` array fields must have **exactly 7** elements for the new mode; legacy mode still expects **exactly 2**.
- Dates must be actual `YYYY-MM-DD` calendar dates and one day apart, including month/year rollovers and February 29 in leap years. The first date must align to `current.time`'s date.
- All temperatures must be finite, within the pre-existing `[-100,100] C` bounds, with `max >= min`. Weather code must be 0–99 and precipitation probability an integer 0–100.
- A missing, extra, malformed or out-of-order row rejects the **whole candidate snapshot**; `lastValidSnapshot()` retains its previous valid content and no invented forecast is exposed.
- `WeatherService::sixDayDateSelfCheck()` covers same-day rejection, skipped day, leap/non-leap February, October→November and December→January.
- No credentials, API keys, insecure TLS option, network server or OTA service is added. Weather poll policy and rendering ownership remain application-controlled.

## Isolated Serial-only test

Update `phase8a1-static-prototype`. In ignored **local** `config.h`, keep real `WIFI_SSID`, `WIFI_PASSWORD`, `WEATHER_LATITUDE` and `WEATHER_LONGITUDE` and enable **only**:

```cpp
// #define EDP_PHASE8A1_PREVIEW 1
// #define EDP_PHASE8_FONT_TEST 1
// #define EDP_PHASE8_WEATHER_ICON_TEST 1
// #define EDP_PHASE8B2_CLOCK_TEST 1
// #define EDP_PHASE8B3_MARKET_TEST 1
// #define EDP_PHASE8B4_WEATHER_TEST 1
// #define EDP_PHASE8B5A_WIFI_TEST 1
// #define EDP_PHASE8B5B_APP_TEST 1

#define EDP_PHASE8C1_DAILY_TEST 1
```

Compile/upload `firmware/CrowPanelDashboard/CrowPanelDashboard.ino` with Arduino IDE and watch **115200 baud** Serial Monitor. It is deliberate that the E-paper panel **continues showing its old image**: this mode only fetches/validates real API data and prints to Serial, without drawing pixels or activating Phase 7 buttons/market services.

Expected output example (dates and values are illustrative examples of **log format**, not fabricated live weather):

```text
EDP Phase 8C-1: REAL OPEN-METEO SIX-FUTURE-DAY DATA
No E-paper refresh in this test; previous panel image stays.
PASS: 8C-1 date/leap/year and WMO self-check.
8C-1: HTTPS Open-Meteo forecast_days=7, timezone=Asia/Taipei.
8C-1 provider local today: YYYY-MM-DD
+1d YYYY-MM-DD | [CONDITION] (WMO [code]) | max ... C min ... C | precipitation max ...%
+2d ...
+3d ...
+4d ...
+5d ...
+6d ...
PASS: 8C-1 SIX REAL FUTURE DAYS VALIDATED (serial only).
```

A failed response logs `FAIL: 8C-1 7-day fetch/validation: ...` and retries no faster than once per **30 seconds** while Wi-Fi remains connected. On success this test stops fetching; it has no autonomous visual refreshing.

## Physical acceptance checklist

- [ ] Arduino compile/upload and successful connection with **verified HTTPS root certificate** (pre-existing client).
- [ ] Serial log shows date/calendar + WMO self-check PASS.
- [ ] Six printed `+1d` ... `+6d` lines are **real consecutive future dates** with plausible daily WMO, max/min Celsius and maximum precipitation probability.
- [ ] Serial displays `PASS: 8C-1 SIX REAL FUTURE DAYS VALIDATED` and does not show `FAIL`.
- [ ] Panel does not refresh or show old mock daily forecast in this **serial-only** mode. It may retain its previous E-paper image; that is expected.
- [ ] Older Weather page, existing normal Dashboard mode and TLS validation remain unmodified by source review; a complete user hardware regression on those paths is deferred to 8C-3.

**Next, after the reported 8C-1 device PASS:** Phase 8C-2 creates bounded six-column 17px weekday / 32px weather icon / 14px max-min °C and precipitation probability presentation using actual future-day values; then Phase 8C-3 integrates it into the app-owned refresh/dirty cycle. Full-system multi-hour/overnight endurance is **last Phase 8**, not this step.

## Phase 8C-1 actual device acceptance — 2026-10-11

**User confirmation:** After being asked to upload the exclusive `EDP_PHASE8C1_DAILY_TEST=1` Serial-only experiment and verify Open-Meteo's real `forecast_days=7` response, the user replied **「實際測試正常」**.

**Accepted scope:** Phase 8C-1 real ESP32-S3 data-acquisition / strict seven-date forecast contract test **PASS for reported normal device operation**. The code acquires local today plus six prospective dates through the existing certificate-verifying HTTPS client and uses an atomic validated snapshot. The six-column E-paper graphics/UI integration **has not yet been implemented**; no E-paper redraw is expected in this serial-only test.

**Evidence boundary:** No verbatim Serial transcript, individual six-date values, screenshot, TLS handshake trace or raw weather response was included with the confirmation; do not assert those artifacts were separately reviewed. The user-reported test outcome is sufficient to accept this implementation gate while retaining further data-format edge-case regression for later if needed.

**Next — Phase 8C-2:** Convert `WeatherSnapshot::futureDays[6]` into bounded **weekday / condition bitmap / max-min Celsius / precipitation** presentation fields and render in the existing six 128px columns, using 17px/14px fonts and approved 32×32 assets. Retain the known-good legacy static preview as a fixture, and retain truthful `UNAVAILABLE` placeholders when validated six-day content does not exist. Phase 8C-3 subsequently connects the new daily fields to the 30-minute application weather poll and successful-refresh-only visible dirty lifecycle.

**Git action:** Documentation-only physical acceptance; `WeatherService`, app firmware, TLS roots, `CrowEPD579`, and formal `main` are unchanged by this acceptance.
