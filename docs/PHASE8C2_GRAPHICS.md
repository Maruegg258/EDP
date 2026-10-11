# Phase 8C-2 — Real six-day forecast graphics hardware gate

**Prepared:** 2026-10-11  
**Branch:** `phase8a1-static-prototype`  
**Status:** Source implementation and GitHub review PASS. **User reports normal real CrowPanel results for the isolated 8C-2 six-column graphics test (2026-10-11); hardware gate ACCEPTED for tested normal operation.** Detailed Serial transcripts/screenshots were not supplied. Phase 8C-1 real-data contract previously passed.

## Purpose and inputs

Draw exactly **tomorrow through day +6** from the already accepted `WeatherService::fetchLatestSixDay()` response: `WeatherSnapshot::futureDays[6]`, with `hasSixDayForecast=true`. Do **not** show today's weather as the first future day. This is an **isolated one-shot E-paper typography check**, not Phase 8C-3 production app polling.

The standalone renderer is `Phase8A1Preview::renderWithSixDayForecast()`, which uses the same Phase 8A/8B existing 792×272 graphics compositor. Native JetBrains Mono glyph assets and fixed approved anchors remain untouched: six columns, **128px wide**, left origin **x=12**; each 32×32 icon at **y=200**; 17px weekday beside icon at **y=209**; 14px summary centered at **y=243**.

| Visible element | Source | Format |
| --- | --- | --- |
| Weekday | `WeatherDailyValue::date` converted via Gregorian weekday | `MON`, `TUE`… `SUN`, 17px |
| Daily condition icon | Normalized WMO `WeatherDailyValue::condition` | Existing approved 32×32 icon. Daily code has **no isDay**, so use daytime representative artwork (not a sunrise/sunset prediction) |
| Max / min temperature | `temperatureMaxC`, `temperatureMinC` | Nearest integer °C, e.g. `29/25C` (MAX/MIN); unchanged underlying float values |
| Maximum precipitation probability | `precipitationProbabilityMax` | `80%` appended to summary: `29/25C 80%`, 14px |

The shared 14px font supports digits, `/`, `C`, space and `%`; its 9px fixed cell gives **at most 12 glyphs = 108px** inside the 128px column, leaving at least 10px margin per side. The renderer separately measures actual bitmap text width; does not truncate a number or silently omit precipitation. If unusual but parsed-valid values (such as extreme negative temperatures) cannot fit, the presenter **rejects the whole new frame** and preserves any previously accepted content rather than drawing clipped/misleading values. Formatting is deliberate and may be revisited based on actual on-panel visual feedback.

## Data/graphics ownership and regressions

- `Phase8SixDayForecast` owns 6 short weekday/summary character arrays and immutable icon pointers; `fromSnapshot` does not overwrite the caller on failure, compares complete rendered state through `equal`, and rejects missing six-day validity, invalid dates, nonconsecutive daily positions, out-of-range values or 14px column overflow.
- `Phase8SixDayPresentation::selfCheck()` covers a known Monday–Saturday sequence, correct integer rounding and formatted summary, loss of `hasSixDayForecast`, invalid weather values, duplicate/skipped dates, leap day and year rollover.
- The historical **static** Phase 8A mock/Rev-D2 renderer retains the same six fictional fixture columns *only in that deliberately opt-in diagnostic*. The already accepted **8B-5B normal app route remains unchanged**, still showing `6-DAY FORECAST UNAVAILABLE`; it does **not** receive a hidden new data source yet.
- E-paper ownership remains with application code. Isolated 8C-2 calls the **verified `maintenanceRefresh(frameBuffer)` exactly once after a successful forecast fetch**, then copies `previousFrameBuffer` and sleeps. No new full-refresh cycle, driver changes, Web server, OTA, BLE, MQTT or `setInsecure()`.
- The isolated mode uses real Wi-Fi RSSI and Open-Meteo current conditions on top, but does **not** poll NTP or Binance, so header clock/date and Crypto are intentionally **`WAITING` / `--` placeholders**, not a regression in those production services.

## Enable isolated test

Pull the `phase8a1-static-prototype` branch, retain local git-ignored `config.h` Wi-Fi secrets and weather coordinates; comment out **every prior preview/application test flag** and enable:

```cpp
// #define EDP_PHASE8A1_PREVIEW 1
// #define EDP_PHASE8_FONT_TEST 1
// #define EDP_PHASE8_WEATHER_ICON_TEST 1
// #define EDP_PHASE8B2_CLOCK_TEST 1
// #define EDP_PHASE8B3_MARKET_TEST 1
// #define EDP_PHASE8B4_WEATHER_TEST 1
// #define EDP_PHASE8B5A_WIFI_TEST 1
// #define EDP_PHASE8B5B_APP_TEST 1
// #define EDP_PHASE8C1_DAILY_TEST 1

#define EDP_PHASE8C2_DISPLAY_TEST 1
```

Arduino IDE: compile/upload `firmware/CrowPanelDashboard/CrowPanelDashboard.ino`; open Serial Monitor at **115200 baud**. The panel retains its previous image until Wi-Fi connects, the real 7-date response passes validation and the complete six-column frame can be drawn. Then it performs **one maintenance refresh only**; nothing updates further in this isolated mode.

Representative Serial output:

```text
EDP Phase 8C-2: SIX REAL DAILY WEATHER COLUMNS
Upper calendar/market fields are WAITING/-- placeholders.
PASS: 8C-2 weekday, temperature, precipitation and icon self-check.
8C-2: requesting 7 verified daily records from Open-Meteo...
PASS: 8C-2 SIX REAL FORECAST COLUMNS DRAWN ONCE.
  +1d 2026-10-12 -> MON 29/25C 80%
  +2d ...
  +3d ...
  +4d ...
  +5d ...
  +6d ...
```

Dates/temperatures in the **example** are only format illustrations. The actual screen/log must come from live Open-Meteo data and the configured user location; never assume the example values represent the forecast.

## Physical acceptance criteria

- [x] GitHub implementation/source inspection confirms six bounded columns and preserved old renderer/driver/HTTPS.
- [ ] Sketch compiles and uploads in Arduino IDE with **only** `EDP_PHASE8C2_DISPLAY_TEST=1`.
- [ ] Serial self-check PASS, successful real API acquisition and **`PASS: 8C-2 SIX REAL FORECAST COLUMNS DRAWN ONCE.`**
- [ ] Physical six columns are legible; weather icon + weekday and max/min°C + rain probability fit without colliding or clipping.
- [ ] Correct consecutive future weekdays, corresponding icons, plausible forecast values. No false `MON`–`SAT` static fixture, and no new deliberate center-seam artifact.
- [ ] Upper header/price placeholders are understood as part of this isolated test; the test is **not** a complete live Dashboard.
- [ ] No unexpected repeated refresh; next application integration remains pending for **8C-3**.

**8C-3 (NOT IMPLEMENTED)** will supply the six-day forecast to the normal Phase 7 application, reusing the 30-minute weather poll and last-valid semantics, keeping UI/data update separated and coalescing the six-day display's visible dirty state. Full-feature long-duration/overnight endurance stays deferred to the **end of Phase 8** per user request.

## Phase 8C-2 real CrowPanel hardware sign-off — 2026-10-11

**User feedback:** After the Phase 8C-2 single-refresh device test was requested, the user replied **「實際測試正常」**.

**Accepted:** The 8C-2 isolated real-forecast layout is accepted for **reported normal physical panel operation**. The six 128px columns represent **tomorrow through day +6** with existing native 32×32 daytime-representative weather icon, 17px weekday, and 14px summary formatted as nearest-integer max/min °C plus maximum precipitation %. This test does **not** enable dynamic forecast polling inside the Phase 7 normal application.

**Evidence limit:** No specific screenshot, six dated provider values, compilation transcript, precise Serial PASS trace, forced negative-temperature edge case, or duration beyond this one-shot device test was supplied. Accept the overall hardware report without inventing individual proofs or implying the separate app Dirty Tracking has been verified.

**Next — Phase 8C-3:** After entering the normal application, update its existing 30-minute WeatherService poll to obtain the validated seven-date response, stage the owned six-day presentation alongside calendar/current weather/markets/Wi-Fi, compare only visually rendered forecast state, keep last-valid forecasts on transient failure, and commit pending state **only after** successful partial/maintenance refresh or page-return render. Test Dashboard ↔ Weather/Markets, DETAIL DISPLAY CLEAN, Deep Sleep and repeated weather updates, while retaining the accepted SSD1683 maintenance sequence and certificate validation. No code modifications were made by this 8C-2 hardware acceptance checkpoint.

**Final endurance:** The user explicitly postponed multi-hour ghosting/overnight transitions until all Phase 8 Dashboard functionality is finished. Keep that gate pending.

## Phase 8C-3 application integration now available (2026-10-11)

The independently user-accepted 8C-2 six-column renderer is now used by `Dashboard::renderLive()` through the existing `PageRenderer` and application refresh flows when **both** 8B-5B normal app and 8C-3 forecast flags are enabled. The forecast is stored within `Phase8DashboardFrame` and only commits after physical refresh. No 8C-2 icon/text placement or provider parser changes occurred during this step. Device gate remains pending. See [PHASE8C3_INTEGRATION.md](PHASE8C3_INTEGRATION.md).
