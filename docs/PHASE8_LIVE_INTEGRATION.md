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

## Phase 8B-3 — Actual three-price market slice (2026-10-11)

An opt-in isolated `EDP_PHASE8B3_MARKET_TEST` now combines the **existing** NTP time and `MarketDataService` (BTCUSDT/ETHUSDT/HYPEUSDT on the already verified USDⓈ-M perpetual endpoint). A bounded `Phase8MarketPrices` struct owns 3 display strings (≤10 glyphs each), and `Phase8MarketPresentation::formatPrice()` converts full-precision last-valid ASCII prices to the user-approved 34px presentation without modifying provider values. Max preferred decimal digits: BTC 1, ETH 2, HYPE 3, reduced if the integer grows. Decimal half-up carry and refusal to truncate oversized integer parts are checked. In the test mode, a single app-side refresh follows successful canvas composition of the combined NTP/price candidate state. `Dashboard::render()`, `DashboardUpdateCoalescer`, `WeatherService`, main branch, TLS trust roots and the SSD1683 driver remain unchanged.

The approved six-day values and current weather are still static fixtures; the top-right Wi-Fi symbol is still a mock STRONG indicator even though the underlying Wi-Fi connection is real for HTTPS. See [PHASE8B3_MARKETS.md](PHASE8B3_MARKETS.md) for the integration scope, display precision and device tests. This is **not** yet the production Dashboard renderer.

## Phase 8B-3 device acceptance → Phase 8B-4 handoff (2026-10-11)

User reports the opt-in Phase 8B-3 NTP and three-market firmware ran normally on the actual CrowPanel. This satisfies the **isolated on-panel device acceptance gate**. Do not infer extended fault-injection, performance/endurance, midnight rollover or every stress scenario from the general success response.

For Phase 8B-4, carry forward the accepted live calendar/price presentation; map `WeatherSnapshot::current` (including `isDay`) using the approved eleven-icon selector and visible-text formatting, without modifying the service's verified TLS, 30-minute cadence or ownership boundaries. Keep one application-owned render/refresh path in the test mode; defer genuine six-day daily values to 8C and normal `DashboardState`/page integration to 8B-5.

## Phase 8B-4 isolated current-weather implementation (2026-10-11)

The user-approved weather artwork is connected to `WeatherSnapshot::current` in an **isolated test mode** using `Phase8WeatherPresentation` (bounded label, Celsius temperature, immutable weather bitmap selected by `condition`+`isDay`). Its visible equality includes the icon pointer so night/day changes are not lost. The new `Phase8A1Preview::renderWithCalendarMarketsWeather()` reuses all accepted layout coordinates and the Phase 8B-3 public HTTPS NTP/market staging loop; the new 30-minute weather poll uses the existing TLS-validating `WeatherService`.

Only when candidate pixels differ does the application run the already verified restore-previous/partial-refresh path; displayed state is recorded after success. Tests in `docs/PHASE8B4_WEATHER.md`. Production `Dashboard::render()`, `WeatherWidgetMapper`, `PageRenderer`, `DashboardUpdateCoalescer`, services and driver remain unchanged. Physical device acceptance remains pending.

## Phase 8B-4 hardware acceptance and Phase 8B-5 handoff (2026-10-11)

User confirms the isolated Phase 8B-4 NTP + public perpetual markets + current Open-Meteo weather Dashboard displays/operates normally on real CrowPanel hardware. The stage's **normal on-device behavior is accepted**; dedicated day/night, data-outage, 30-minute repoll, cross-midnight and endurance/ghosting tests are not established by this general reply and remain later regression work.

Phase 8B-5 should replace the mocked top-right Wi-Fi icon with existing `WiFiManager` RSSI semantics; carefully integrate the shared approved presentation into production `Dashboard::render()`/`PageRenderer` and **every application-owned physical display path** with correctly owned state and coalesced dirty comparisons. Preserve Phase 7 DETAIL actions and deep sleep, verified maintenance refresh sequence, HTTPS certificate checks and the approved 8A layout. Six-day mocks must not be falsely represented as real forecasts in production; solve this deliberately before the production route is enabled.

## Phase 8B-5A — RSSI prototype accepted-scope implementation (2026-10-11)

The 8B-4 approved isolated live NTP/market/current-weather Dashboard now supports real upper-right Wi-Fi status via an opt-in `EDP_PHASE8B5A_WIFI_TEST`. Existing `WiFiManager` state and Phase 4 RSSI categories directly provide five immutable 32×24 bitmaps, which are fed to the **same** graphics-only Phase 8A compositor. Sampling is 60 seconds plus immediate link-state changes, and selected bitmap equality participates in the common displayed-versus-candidate compare. The application owns physical refresh and commits the displayed Wi-Fi pointer after E-paper success, not before. The old 8B-4 and earlier mock Wi-Fi preview entry points remain.

**Phase 8B-5B is not yet implemented.** It will require a deliberate production UI/rendering and owned dirty/coalescer integration including all Phase 7 page refresh / DETAIL system actions. It must also avoid presenting the current six mock daily forecasts as real predictions. See [PHASE8B5_WIFI.md](PHASE8B5_WIFI.md). 8B-5A device compile/upload and hardware acceptance pending.

## Phase 8B-5A device sign-off → 8B-5B production design gate (2026-10-11)

User reports the isolated live NTP + USDⓈ-M markets + current Open-Meteo + dynamic Wi-Fi RSSI Dashboard test is **normal on the physical CrowPanel**, meeting the 8B-5A normal-operation device gate. This does not constitute a deliberate disconnected/reconnected or signal-level boundary test; these remain 8B-6 tests.

8B-5B is **not implemented**. Before enabling the new UI on normal app paths, inspect the existing `DashboardState`, `DashboardStateSnapshot`, `DashboardUpdateCoalescer`, `PageRenderer` and all app-owned refresh entry points, including `DISPLAY CLEAN` and deep sleep. Ensure calendar/day-night/price/RSSI visible state is owned, dirty-compared, and committed after physical display success. Keep the `main` branch and existing panel driver untouched until this next gate is implemented and physically accepted. The six-day strip still shows static demo data; production rendering must use clearly unavailable placeholders or real approved six-day data, never the mock forecast.

## Phase 8B-5B — Application route and visible-diff integration (2026-10-11)

A bounded, owner-copyable `Phase8DashboardFrame` now carries calendar (including actual date/weekday), display-rounded three perpetual prices, current condition/day-night icon and RSSI-selected bitmap. `Phase8DashboardVisualState` distinguishes physically displayed versus pending appearance and only commits after a successful panel refresh. This is layered on the **existing** production `DashboardUpdateCoalescer`, which still stages legacy widgets/services; for the opt-in modern Dashboard, redundant old raw/status dirty alone does not repaint the panel.

The normal Phase 7 application loop (NTP, MarketDataService, WeatherService, WiFiManager, page buttons) is still the **sole** fetch/refresh owner. For the 8B-5B test flag it renders `Phase8A1Preview::renderLive()` through `Dashboard::renderLive()` and optional `PageRenderer::useLiveDashboardFrame()`, then reconciles staged/displayed state across startup, partial, page return and DISPLAY CLEAN. Normal standalone 8A and 8B-2..5A test modes, legacy app route without the flag, WEATHER/MARKETS pages, DETAIL system actions, verified maintenance sequence and TLS roots remain intact.

**No fake forecasts:** Live lower strip now shows `6-DAY FORECAST UNAVAILABLE`. Former static MON–SAT / repeated `25-29/ 10%` values appear **only** in explicitly selected historical previews, never this live app. Device compile/hardware regression is still pending. See [PHASE8B5B_PRODUCTION.md](PHASE8B5B_PRODUCTION.md).

## Phase 8B-5B hardware sign-off → Phase 8B-6 (2026-10-11)

After loading and testing the opt-in modern Dashboard inside the original Phase 7 application, the user reports **「實際測試正常」**. Phase 8B-5B is therefore **accepted for normal observed physical CrowPanel operation**, on the feature branch. Existing NTP/market/current weather/Wi-Fi UI inputs, PageRenderer routing, the same shared modern artwork and the honest six-day-unavailable caption form the accepted baseline.

Do not infer that every requested DETAIL/standby route, forced failure, physical midnight rollover, strict no-change suppression or extended E-paper endurance was separately observed. The next formal gate is **Phase 8B-6**, with targeted integration and refresh regression before considering a production-main promotion. No firmware files, HTTPS trust validation, network services or SSD1683 behavior were modified in this documentation-only sign-off.

## Phase 8B-6 staged reliability regression (2026-10-11)

At the 8B-5B user-accepted HEAD `546c9de0697ebb656a2cbcda34bb5c08cb1165b1`, source-level paths for app-owned refresh, visual-diff dirty comparison, NTP calendar/date/weekday, day/night bitmap, RSSI grade, unchanged raw-price rounding, TLS trust and validated SSD1683 maintenance sequence were re-read. No new issues requiring a speculative driver or display-sequence change were confirmed by this inspection, **but code review cannot prove physical device behavior**.

A reproducible physical regression protocol is in [PHASE8B6_REGRESSION.md](PHASE8B6_REGRESSION.md): 8B-6A page/DETAIL/DISPLAY CLEAN/standby with following partials; 8B-6B controlled Wi-Fi outage/recovery and normal polling; 8B-6C long-run partial quality and actual date/day-night transitions. Only 8B-6A should be requested first so a failure can be isolated without overlapping multiple stress conditions. No firmware changes or new network capabilities are included in the protocol checkpoint.

## Phase 8B-6A device regression sign-off (2026-10-11)

The user explicitly confirms that **all three** short regression procedures operate normally on the actual CrowPanel: page switching, DETAIL DISPLAY CLEAN with the three subsequent minute partial updates, and STANDBY/deep sleep/wake. This supports normal 8B-6A acceptance. Do not infer a physically observed midnight rollover, forced RSSI boundary, Wi-Fi/TLS failure recovery or multi-hour ghosting performance. Phase 8B-6B network/regression and 8B-6C endurance remain pending; no firmware source is changed by this sign-off.

## Phase 8B-6B real-device network/recovery sign-off (2026-10-11)

The user replied **「實際測試三項都正常」** to the 8B-6B set: temporary Wi-Fi disconnect/last-valid+clock handling, subsequent Wi-Fi reconnect/data updates, and service polling/visual dirty observation. Mark the 8B-6B normal-use physical gate PASS on the accepted feature-branch firmware. This broad outcome is not a detailed packet or E-paper refresh event log and does not individually validate every RSSI threshold or service timestamp. The next gate is 8B-6C 1–2-hour E-paper/dual-controller-seam endurance with post-maintenance partials, optionally real local midnight and sunrise/sunset if actually observed. Keep untested transitions explicitly pending/deferred; do not alter SSD1683 or verified TLS merely to provoke them.

## Phase 8C-1 — Open-Meteo seven-date contract (2026-10-11)

After 8B-6A/B hardware sign-offs, the user moved the 8B-6C long-term/overnight endurance test to **the final Phase 8 validation**, after six-day forecasts and the final UI are live. This is a scheduling decision, not an endurance PASS.

The existing `WeatherService` gained a **separate opt-in** `fetchLatestSixDay()` request with `forecast_days=7`, bounded six future-day fields and calendar/date/temperature/probability/WMO validation. Original production `fetchLatest()` still requests two daily dates and the existing WEATHER page/30-minute poll uses it unchanged. An isolated serial-only test flag requests and displays the six real future days after HTTPS CA validation. No second weather parser, fake daily data, E-paper refresh, new network service or key is introduced. 8C-2 graphics and 8C-3 final app/UI binding remain gated on actual 8C-1 hardware acceptance. See [PHASE8C_DAILY.md](PHASE8C_DAILY.md).

## Phase 8C-1 hardware acceptance → 8C-2 graphics gate (2026-10-11)

The user reported **「實際測試正常」** for the isolated 8C-1 serial-only real seven-date Open-Meteo acquisition and validation. Accept this as a normal-hardware-operation gate, while noting that individual daily values and TLS output were not copied into the chat. The existing 8B-5B opt-in live UI, Phase 7 navigation and 2-day `WeatherService::fetchLatest()` application calls remain unchanged. Proceed to **8C-2** approved six-column text/icon graphics using real `futureDays[6]` without conflating that later rendering work with the now-passed serial-data test. Endurance remains deferred until final Phase 8 complete-feature acceptance.

## Phase 8C-2 isolated six-day rendering (2026-10-11)

Built `Phase8SixDayPresentation` from owned six real `WeatherSnapshot::futureDays` entries. It validates Gregorian date/weekday, glyph budget, condition icon, integer max/min °C and precipitation while retaining existing source decimals, then passes immutable bitmap pointers and bounded text to the **same** Phase 8A/8B UI composition at fixed six-column coordinates. A separate opt-in one-shot mode uses verified Open-Meteo 7-date fetch and app-owned `maintenanceRefresh()`; it does not affect normal Dashboard polling/Weather/Markets/DETAIL pages or the SSD1683 driver. Previous 8B route still displays `6-DAY FORECAST UNAVAILABLE` until 8C-3; source validation only, physical screen test pending. See [PHASE8C2_GRAPHICS.md](PHASE8C2_GRAPHICS.md).
