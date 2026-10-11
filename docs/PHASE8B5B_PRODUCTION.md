# Phase 8B-5B — Modern Dashboard in the normal application

**Date:** 2026-10-11  
**Branch:** `phase8a1-static-prototype`  
**Current status:** Code committed, GitHub readback/source-level checks PASS. **ESP32-S3 Arduino compile/upload and hardware acceptance PENDING.** `main` stays unchanged.

## Why 8B-5B is separate from the passed 8B-5A

The 8B-5A preview is an isolated polling loop with no buttons, PAGE/DETAIL navigation, WEATHER/MARKETS pages or deep sleep. Phase 8B-5B **does not add a new polling stack**: it puts the *same user-approved 34/17/14px layout* inside the existing application state/refresh lifecycle. It is still an **explicit opt-in feature-branch gate** pending a real-device regression test, so turning the test flag off continues to use the Phase 7 Dashboard.

### Owned visible presentation and dirty semantics

`Phase8DashboardFrame.h/.cpp` introduces fixed-size, value-owned and shallow-static-bitmap-only state:

- `Phase8CalendarHeader` (HH:MM, DD MMM, complete weekday).
- `Phase8MarketPrices` (3×11 bounded 34px strings, displayed order ETH/BTC/HYPE).
- `Phase8CurrentWeather` (bounded 17px condition label, 14px Celsius, immutable approved 32×32 icon).
- `const Bitmap1bpp* wifi` (immutable 32×24 Phase 4 RSSI/connection bitmap).

`Phase8DashboardVisualState` has **displayed** and **pending** owned frames, compares only the visible text/selected artwork and commits pending **after physical E-paper success**. The normal app keeps its already verified `DashboardUpdateCoalescer` for its legacy data and service staging. The modern rendered Dashboard uses the additional bounded *visible-diff* state so a midnight date/weekday change, sun↔moon artwork or RSSI grade transition is not missed merely because the old widget dirty mask lacks that information. A raw market price change that rounds to the same 34px display, a provider timestamp change or an RSSI fluctuation inside the same bitmap grade does not require a modern Dashboard-only physical refresh. The same service data remain available to the existing Weather and Markets pages.

After the old application polls run (60-second Binance public USDⓈ-M symbols, 30-minute Open-Meteo TLS weather, production Wi-Fi/TimeService), the application assembles a bounded candidate and stages it. There is no independent network, refresh timer or new `WeatherService`. Page switches, normal partial refresh and DISPLAY CLEAN all compose the latest candidate and commit the matching displayed state **only after successful physical display**.

### One shared compositor with a truthful forecast state

`Phase8A1Preview::renderLive(GraphicsBW&, const Phase8DashboardFrame&)` uses the *same* Phase 8A header/price artwork, dividers, native font assets, condition icons and Wi-Fi symbol. It differs from the standalone mock `render()`/8B-2/3/4/5A preview **only in the lower forecast content**:

**Production route draws: `6-DAY FORECAST UNAVAILABLE`.**

The Phase 8A Monday–Saturday mock daily rows, repeated `25-29/ 10%` samples and example icons are not presented as current predictions in a live application. `WeatherSnapshot` still has only today + tomorrow daily data, so six real daily forecasts must be collected separately in Phase 8C. The accepted six-column typography/icons remain available in the historical opt-in static previews, without duplicating their renderer.

### Render and controller entry points

| Application entry point | 8B-5B behavior |
| --- | --- |
| Boot baseline, `establishBaseline()` | Render modern WAITING/--/NO DATA/disconnected frame; for **this opt-in only**, use the already accepted `maintenanceRefresh()` baseline, **not an extra full refresh**, then record physical state |
| Old application production poll + `stagePhase8LiveDashboardFromServices()` | Read existing last-valid NTP/market/current-weather and RSSI; stage owned visible frame only (no E-paper operation) |
| `flushPendingPartial()` / `flushPendingMaintenance()` | Same pre-existing driver sequence; select modern frame when enabled and commit both application and visual snapshots after physical success |
| `PageRenderer::render(PageId::DASHBOARD)` | Optionally uses `Dashboard::renderLive()` with latest frame. WEATHER and MARKETS still use old verified renderers |
| Page switch / DETAIL MENU / EXIT | Original `NavigationController` and `SystemPages` remain active; returning to Dashboard renders the newest pending frame and commits after update |
| DETAIL `DISPLAY CLEAN` | Render current modern Dashboard (never the menu frame), then original verified maintenance refresh; exit DETAIL after success |
| DETAIL `STANDBY` | Draw the same small moon and use the already verified EXT1 deep sleep/wake / Wi-Fi off flow |

### Test setup in local `config.h`

Update the `phase8a1-static-prototype` branch. Keep real `WIFI_SSID`, `WIFI_PASSWORD`, `WEATHER_LATITUDE` and `WEATHER_LONGITUDE` in your **ignored local** `config.h`. Disable every previous Phase 8 preview flag:

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

Compile/upload `firmware/CrowPanelDashboard/CrowPanelDashboard.ino` in Arduino IDE and open **115200 baud** Serial Monitor. Both coordinate macros must be defined, or compilation intentionally stops with an explanatory `#error`.

Expected boot messages include:

```text
PASS: Phase 7D navigation/menu self-checks.
PASS: Phase 3 state/coalescing regression self-checks.
EDP Phase 8B-5B: normal Phase 7 app with approved live Dashboard.
Six-day forecast: UNAVAILABLE, not mock daily predictions.
PASS: Phase 8B-5B owned state and presentation self-checks.
Establishing live dashboard baseline...
```

Subsequently the same normal production Binance, WeatherService and NTP logs should appear as their due times occur. There is no new HTTP API or MQTT/BLE/OTA service.

### Device acceptance checklist

- [ ] **Compile/upload:** Arduino ESP32-S3 build succeeds with `EDP_PHASE8B5B_APP_TEST 1` and passed serial self-checks.
- [ ] **Home page:** accepted native 34/17/14px date/time/price/weather/wifi layout is live; no text collision, ghosting or controller seam artifact.
- [ ] **Forecast truth:** six bogus daily values are gone; lower strip says `6-DAY FORECAST UNAVAILABLE`.
- [ ] **Page navigation:** UP/DOWN switches DASHBOARD → WEATHER → MARKETS and back; WEATHER and MARKETS retain their verified data.
- [ ] **DETAIL menu:** MENU opens DETAIL, UP/DOWN chooses STANDBY or DISPLAY CLEAN, EXIT returns correctly without stale content.
- [ ] **DISPLAY CLEAN:** performs verified maintenance sequence and redraws the **modern live Dashboard**, not the menu or the older Phase 7 Dashboard. The next minute partial refresh stays crisp.
- [ ] **STANDBY:** renders the small moon and enters deep sleep with Wi-Fi off; MENU/EXIT wakes and restarts the correct Dashboard.
- [ ] **Live values:** NTP rolls minute, BTC/ETH/HYPE values update, current Open-Meteo condition/temperature/day-night bitmap remains correct, Wi-Fi status changes render.
- [ ] **Visible dirty:** no refresh on unchanged visible state; combined near-simultaneous service changes are rendered once; state/previous buffer advances only after physical success.
- [ ] Optional deliberate Wi-Fi outage, midnight calendar boundary, repeated 30-minute weather poll and prolonged ghosting tests can be followed up as Phase 8B-6.

### Known limits and safety boundaries

- This is an **opt-in normal-app device validation route**, not a merge into `main`. The original Phase 7 Dashboard remains unchanged when the flag is off. `PageRenderer` uses the modern frame only when the application explicitly supplies it.
- Unlike the prior isolated 8B-5A test, this route **does run all the original Phase 7 button and page actions**. Check standby and DISPLAY CLEAN in particular before declaring hardware accepted.
- The modern frame has its own visible-diff ownership layered on the existing production `DashboardUpdateCoalescer`; it does not ask services to draw the E-paper.
- At this stage a six-day forecast is genuinely **unavailable**. Phase 8C must retrieve enough real daily records, validate condition/temp/precip/date and replace the caption only after hardware acceptance.
- Existing secure HTTP trust anchors, API endpoints, source services, market/weather polling cadences, dual-controller driver, physical partial refresh, fast clear/physical white/previous RAM synchronization and Phase 7 sleep hardware behavior have not been modified.
- No API key or Wi-Fi secret belongs in GitHub. This commit includes only `config.example.h`, not user `config.h`.

**Phase 8B-5B coding checkpoint complete; real hardware acceptance pending.**
