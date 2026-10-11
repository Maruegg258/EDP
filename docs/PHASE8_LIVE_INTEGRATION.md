# Phase 8B-1 — Live Dashboard interface audit and integration contract

**Date:** 2026-10-11  
**Reference:** `Maruegg258/EDP`, branch `phase8a1-static-prototype`, starting commit `333ff6954b54a91e2087a7e07193cd4f7269192e`. Baseline `main` at review: `0471c1e63c218ecad85375e671cee674476db7f9`.  
**Status:** Source-level architecture audit and design only; **no firmware edits, Arduino compilation or live hardware testing in Phase 8B-1**.

## Why this checkpoint exists

Phase 8A produced a visually accepted **static, one-shot** 792×272 dashboard, with JetBrains Mono Medium 34/17/14px text, user-approved 11×32px weather icons, three centered ETH/BTC/HYPE markets, and six fixed illustrative forecast columns. `Phase8A1Preview::render(GraphicsBW&)` has no live inputs, polling, application dirty tracking or navigation.

Production `Dashboard::render(const DashboardState&)` remains the **older Phase 7 layout** with crypto-logo cards, border, and status strip. Merely removing `EDP_PHASE8A1_PREVIEW` does **not** make the Phase 8A design live. Preserve both the accepted feature-branch artwork and the functioning production path until staged integration has been independently verified.

## Current service contract (verified from source)

| Input / owner | Existing API and semantics | New-layout requirement |
| --- | --- | --- |
| Local time: `TimeService` | `epoch()`, `getLocalTime(tm&)`, `formatLocalTime(...,strftime format)`; app stages `%H:%M` per minute after NTP sync | Native 34px HH:MM plus 17px day/month + full weekday. Date/weekday are **not currently in DashboardState**; format from local `tm` and track displayed changes, including midnight |
| Prices: `MarketDataService` | Three `MarketPriceValue.price[24]` strings for `BTCUSDT`, `ETHUSDT`, `HYPEUSDT`, with last-valid preservation. App stages `CryptoWidgetState.price` through `stageBtc/eth/hype` | Display order **ETH, BTC, HYPE** in the accepted 34px three-column row, without logos/USDT PERP; format source price safely to the available 248px column |
| Current weather: `WeatherService` | `lastValidSnapshot()->current` includes `temperatureC`, `condition`, `weatherCode`, `isDay` | Map `WeatherCondition+isDay` using the approved `WeatherIconSelector` and a 17px caption, 14px temperature; ensure day/night-only icon changes can create dirty |
| Weather widget: `WeatherWidgetMapper` | Produces `WeatherWidgetState` with label, temperature and **legacy SUN/CLOUD/RAIN icons**; current application self-check expects those icons | Phase 8B must not directly swap out mapper assumptions in a broad change. Introduce/validate approved icon semantics in a separate step, update old-path expectations only when intended |
| Wi-Fi: `WiFiManager` | `isConnected()`, `rssi()`; app uses `wifiIconForRssi()`/disconnected staging | Reuse existing `WiFiWidgetState.icon`; accepted top-right position stays fixed |
| Daily forecast: `WeatherSnapshot` | `today` + `tomorrow`, each `WeatherDailyValue` with condition/min/max/precip probability; `DAILY_RESPONSE_COUNT=2` | **Six real day rows do not exist**. Work belongs to later Phase 8C, not disguised by static MON-SAT fixtures |

Existing hourly forecast = five `futureHours[]`; this is not the six-day daily forecast. For an incomplete live Dashboard, show explicit unavailable/empty forecast cells or keep the live mode disabled until Phase 8C; **never publish the fixed `25-29/ 10%` samples as actual predictions**.

## Presentation ownership boundary

1. Services own provider access, certificate-validation and last-valid data; they **must not** own `GraphicsBW` or E-paper refresh.
2. A narrow **application/presentation adapter**, to be designed in 8B-2, turns stable state or service snapshots into a bounded, **owned** visual frame: local `HH:MM`, uppercase `DD MMM` and weekday, weather label and temperature, static-lifetime `Bitmap1bpp` pointers, three display-price strings, Wi-Fi icon, and ultimately daily forecast cells. Calendar formatting must be locale-independent or explicitly validated. For dynamic data, no pointer may outlive its temporary stack/string owner.
3. Extract or parameterize the **existing** accepted `Phase8A1Preview` drawing logic into one reusable graphics-only renderer. Keep the current immutable preview fixture as a regression entry point; do **not** build another unrelated `.ino` dashboard/layout. Renderer may report invalid or overflow data; it must not silently clip or silently fake a live value.
4. Use the existing `DashboardStateSnapshot` and `DashboardUpdateCoalescer` as the model for deep-copied visual state and dirty/commit semantics. The current struct owns `_time[8]`, weather label/temperature, three `price[24]`, status and bitmap pointers, but lacks calendar and six-day fields. Add their ownership and dirty detection **incrementally**, or prove an equivalent displayed-versus-pending visual comparison before any dynamic mode. Avoid committing displayed state **before** a successful physical update. No independent periodic refresh task.
5. The application remains the **sole owner of physical E-paper I/O**. Preserve the verified maintenance sequence and prior/current framebuffer sync; keep the current partial/display flush choreography and page ownership. Do not add OTA, Web Server, MQTT, BLE, telemetry or `setInsecure()`.

### Display entry points that must use the same eventual live Dashboard renderer

| Path in current `CrowPanelDashboard.ino` | Why it matters |
| --- | --- |
| `establishBaseline()` | Initial displayed frame and `establishDisplayed` state must agree; current implementation uses a baseline `displayFullFrame()` (existing behavior; **do not introduce another periodic full refresh**) |
| `flushPendingPartial()` | Standard dirty/coalesced Dashboard redraw followed by existing previous-frame restoration / partial refresh |
| `flushPendingMaintenance()` | Existing `maintenanceRefresh()` path when selected by application |
| `flushVisiblePageChangeIfNeeded()` and `PageRenderer::render()` | Dashboard / WEATHER / MARKETS navigation and DETAIL menu transitions |
| `runDisplayClean()` | Phase 7 DETAIL **DISPLAY CLEAN** must redraw the accepted current Dashboard, never a stale mock or menu frame |

The existing `enterStandbyDeepSleep()` must continue rendering its moon and powering off Wi-Fi/panel as already verified; no network should run in deep sleep.

## Specific risks discovered in this audit

- **Calendar dirty gap:** `ClockWidgetState` has only `time`. A new date/weekday field needs owned state/dirty handling; test 23:59→00:00 (including month/year boundaries), timezone and NTP corrections.
- **Day/night weather dirty gap:** Existing `WeatherWidgetMapper` does not use `current.isDay`; `WeatherWidgetState` equality compares only bitmap pointer and label/temperature. If the approved sun/moon icon changes with no temperature/label change, the new icon pointer must be staged and reflected in dirty logic.
- **Daily forecast dirty gap:** `WeatherSnapshotCompare.h` compares only content rendered on the old WEATHER page and daily tomorrow, not a six-day Dashboard strip. Forecast updates must have their own visible comparison when the six-day contract is added.
- **34px price width:** 248px cell minus 24px total guard = 224px usable. At 22px advance, a string has at most **10 glyphs** including `.`, sign and digits. Raw Binance price strings have up to 23 display characters and may exceed the cell. Decide/test deterministic, value-preserving rounding, explicit unavailable/error formatting and bounds **before** enabling live rendering. Do not change exchange precision in `MarketDataService` merely to satisfy the UI.
- **17px weather label width:** A long condition caption must not collide with date/clock in the accepted header. Measure `textWidth()` on every condition; choose concise labels or a documented fallback rather than moving approved anchors without approval.
- **14px daily summary width:** 128px column, 9px advance. The approved fixture `25-29/ 10%` is ten glyphs / 90px. Real negative/three-digit temperature and precipitation cases can be wider; bounds tests required, and unrepresentable values must be handled explicitly.
- **State lifetime:** `DashboardStateSnapshot::capture()` owns bounded copies of text; direct `const char*` to temporary formatted values becomes invalid. Use the same safety for date, weather and any new forecast model.
- **Navigation and refresh coherence:** `PageRenderer` is not the only production Dashboard draw path: direct rendering occurs in baseline, dirty partial, maintenance and DISPLAY CLEAN. All must be brought to one design with correctly committed displayed/pending state. No unrequested periodic full refresh.
- **Unknown/data unavailable:** Existing market/weather retain last-valid; startup uses `--` placeholders. For any new visible field, define placeholder and stale behavior; never portray mock or absent data as live.

## Bounded next development steps and acceptance gates

**8B-2 — Calendar first.** Introduce the minimal owned presentation data contract, retain a static fixture adapter to generate the accepted look, and introduce the real `TimeService` calendar path under an opt-in test route. Test `HH:MM`, `DD MMM`, `SUN/MON…` mapping (full weekday header), time sync failure and 23:59→00:00. All 34/17/14px glyphs and approved icon pixels remain unchanged. Compile + on-panel acceptance before advancing.

**8B-3 — Prices.** Connect and format existing three live last-valid prices in ETH/BTC/HYPE order. Verify all widths and value changes, without touching Binance HTTPS or poll rate. Compile + on-panel confirmation.

**8B-4 — Current weather.** Bridge condition, day/night, temperature and label into the approved icons. Cover identical-condition dusk/dawn changes and last-valid behavior with tests. Compile + on-panel confirmation.

**8B-5 — Wi-Fi and real Dashboard routing.** Use actual RSSI icon, then enable the shared renderer through the current `Dashboard`/application paths while preserving the Phase 7 pages, buttons, MENU DETAIL action, deep sleep, DISPLAY CLEAN and application-only dirty-driven refresh. Test all routes.

**8B-6 — Regression.** Combined 1-minute clock + market + 30-minute weather state refresh, WIFI disconnect/reconnect, day boundary, no-change suppression, controller seam and stale recovery.

**8C — Real six-day data.** Extend the provider contract and snapshots only after the 8B live core is stable. Do not shorten or replace the previously validated WeatherPage hourly/tomorrow functionality.

## Source audit notes

Checked at the above branch HEAD: `CrowPanelDashboard.ino`, `Dashboard.cpp/.h`, `PageRenderer.cpp/.h`, `DashboardState.h`, `WidgetStates.h`, `DashboardStateSnapshot.cpp/.h`, `DashboardStateCompare.cpp/.h`, `WidgetStateCompare.cpp`, `DashboardUpdateCoalescer.cpp/.h`, `DashboardDirty.h`, `TimeService.cpp/.h`, `MarketDataService.h`, `WeatherService.h`, `WeatherWidgetMapper.cpp`, `WeatherSnapshotCompare.h`, `WiFiManager.h`, `Phase8A1Preview.cpp/.h`, `WeatherIconSelector.cpp/.h`, `MarketPage.cpp`, `WeatherPage.cpp`, `DashboardFont14.h`, `DashboardFont34.h`, `PageModel.h`, plus repository `ROADMAP.md`.

**No runtime or hardware behavior was tested or changed by this documentation-only 8B-1 checkpoint.**

## Phase 8B-2 implemented test slice (2026-10-11)

The first coding checkpoint now exists on the isolated Phase 8 feature branch: `Phase8CalendarPresentation` owns and validates `HH:MM` / `DD MMM` / weekday, `Phase8A1Preview` accepts a header parameter but preserves the immutable static fallback, and `EDP_PHASE8B2_CLOCK_TEST` runs the existing Wi-Fi/NTP stack without querying weather/markets. The test mode alone owns its last-physically-displayed calendar state and issues partial refreshes on real visible changes.

**Do not treat this test-only state as the production dirty/coalescer migration.** That remains the next integration gate. Detailed setup and diagnostics: [PHASE8B2_CALENDAR.md](PHASE8B2_CALENDAR.md). New ESP32 compilation and hardware acceptance remain pending.

## Phase 8B-2 hardware acceptance and Phase 8B-3 handoff (2026-10-11)

User confirms the physical Phase 8B-2 live NTP time/date/weekday test **operates normally**. Phase 8B-2 source code is accepted; the on-device 8B-2 gate is closed. Normal production `DashboardState` and `DashboardUpdateCoalescer` are **not** yet migrated to the Phase 8A design. Avoid changing display/refresh ownership when integrating market prices.

Proceed in Phase 8B-3 with existing Binance last-valid three-symbol service, value-preserving bounded price presentation within 224px nominal market text space, unchanged ETH/BTC/HYPE column geometry, and application-owned one-shot/coalesced visual refresh. Keep simulated forecast/weather values clearly distinct from live data.
