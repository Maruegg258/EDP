# EDP Roadmap

This roadmap tracks the progression from a clean hardware driver to the complete CrowPanel dashboard. The order is deliberate: each layer should be verified on hardware before higher-level features depend on it.

## Phase 0 — Project baseline

**Status: Complete**

- Full 8 MB flash backup made before erase
- Development board flash fully erased
- Custom GitHub repository initialized
- Hardware notes recorded
- Security baseline recorded
- Known refresh behavior recorded
- Firmware renamed from `CrowPanelCrypto` to `CrowPanelDashboard`

## Phase 1 — E-paper hardware driver

**Status: Complete**

**Goal:** Own the minimum SSD1683 display stack.

Planned work:

- Verify SSD1683 command sequences against controller documentation
- Implement reset / busy handling
- Implement controller initialization
- Implement white clear
- Implement current-frame RAM writes
- Implement previous-frame RAM writes
- Implement full / fast / partial refresh primitives
- Implement controller sleep
- Verify dual-controller seam/addressing behavior
- Reproduce the known-good maintenance refresh sequence

Hardware checkpoints:

- [x] Phase 1A: reset / BUSY / SWRESET sanity check (verified on hardware, 2026-09-28)
- [x] Phase 1B: Display all white (verified on hardware, 2026-09-28)
- [x] Phase 1C: Display a black geometry + seam mapping test (verified on hardware, 2026-09-28)
- [x] Phase 1D: Display project-owned `HELLO` text (verified on hardware, 2026-09-28)
- [x] Phase 1E-A: Four consecutive partial refreshes without sleep/reset (verified on hardware, 2026-09-28)
- [x] Phase 1E-B: Partial refresh across deep-sleep / hardware-reset cycles (verified on hardware, 2026-09-28)
- [x] Phase 1F: Confirm maintenance refresh does not create blurred follow-up text (verified on hardware, 2026-09-28)

## Phase 2 — Graphics foundation

**Status: Complete**

**Goal:** Build our own compact black/white rendering layer.

Planned work:

- Pixel, line, rectangle primitives
- Text rendering
- Custom font support
- Bitmap/icon support
- Small icon set for Wi-Fi, weather, BTC, ETH, and HYPE
- Region/dirty-state tracking — deferred to Phase 3, where widget change detection and refresh coalescing are introduced

Hardware checkpoints:

- [x] Phase 2A-1: Extract visible-coordinate mapping, pixel/fill-rectangle, and 5x7 text rendering into `GraphicsBW` without changing the Phase 1F refresh sequence (verified on hardware, 2026-09-29)
- [x] Phase 2A-2: Verify `drawLine()` and `drawRect()`, including geometry that crosses the visible x=396 dual-controller seam (verified on hardware, 2026-09-29)

**Phase 2A status: Complete**

- [x] Phase 2B-1: Generic `BitmapFont` abstraction preserves the existing HELLO rendering (verified on hardware, 2026-09-29)
- [x] Phase 2B-2: Expand the base character set and verify dashboard-style typography on hardware (verified on hardware, 2026-09-29)
- [x] Phase 2B-3: Verify mixed Font5x7 + native Font9x13 rendering, including 9-pixel multi-byte glyph rows and text alignment (verified on hardware, 2026-09-29)

**Phase 2B status: Complete**

- [x] Phase 2C-1: Add `Bitmap1bpp` + `GraphicsBW::drawBitmap()` and verify 33x33 multi-byte rows, seam crossing, clipping, and foreground color (verified on hardware, 2026-09-29)
- [x] Phase 2C-2: Add the first project-owned Wi-Fi, weather, and crypto icon assets (verified on hardware, 2026-09-29)

**Phase 2C status: Complete**

- [x] Phase 2D-1: Verify a static dashboard-like frame combining primitives, both fonts, icons, and seam-crossing content (verified on hardware, 2026-09-29)
- [x] Phase 2D-2: Re-run partial + maintenance refresh regression with mixed text/icon content (verified on hardware, 2026-09-30)
- [x] Phase 2D-3: Close out Phase 2 and prepare the repository for the Phase 3 dashboard/widget framework (completed, 2026-09-30)

**Phase 2D status: Complete**

See [docs/PHASE2_GRAPHICS.md](docs/PHASE2_GRAPHICS.md) for Phase 2 graphics verification notes.

## Phase 3 — Dashboard framework

**Status: Complete**

**Goal:** Separate data from presentation and refresh only what needs to change.

Planned work:

- Dashboard layout system
- Widget interface
- Change detection / dirty regions
- Coalescing multiple data changes into one E-paper refresh
- Stable positioning for clock, weather, market prices, and status icons

Hardware checkpoints:

- [x] Phase 3A-1: Extract `DashboardState` and dashboard composition from the sketch into dedicated UI objects while preserving the Phase 2D layout and verified refresh sequence (verified on hardware, 2026-09-30)
- [x] Phase 3A-2: Extract static dashboard regression states from the sketch into a dedicated test fixture so the application layer no longer owns UI test content (verified on hardware, 2026-09-30)

**Phase 3A status: Complete**

Phase 3B checkpoints:

- [x] Phase 3B-1: Extract Clock, Weather, Wi-Fi, and reusable Crypto widgets from `Dashboard` while preserving the Phase 3A layout and verified refresh sequence (verified on hardware, 2026-10-01)
- [x] Phase 3B-2: Finalize the widget-facing composition boundary with widget-specific state objects and a dedicated StatusWidget, preparing widget-local state/change detection for Phase 3C (verified on hardware, 2026-10-01)

**Phase 3B status: Complete**

Phase 3C checkpoints:

- [x] Phase 3C-1: Add widget-state comparison and a logical dashboard dirty bitmask without changing rendering or physical refresh behavior (verified on hardware, 2026-10-01)
- [x] Phase 3C-2: Add a durable previous-state snapshot that owns copied text values instead of retaining live data pointers (verified on hardware, 2026-10-01)
- [x] Phase 3C-3: Use dirty results in the application flow to skip refresh when nothing changed, while preserving full-frame composition and the verified partial-refresh path (verified on hardware, 2026-10-01)

**Phase 3C status: Complete**

Phase 3D checkpoints:

- [x] Phase 3D-1: Introduce refresh coalescing so multiple logical data changes can be merged into one pending dashboard update before a single physical refresh (verified on hardware, 2026-10-01)
- [x] Phase 3D-2: Verify coalesced updates preserve dirty information, durable snapshot ownership, skip-unchanged behavior, and the existing verified refresh path (verified on hardware, 2026-10-01)

**Phase 3D status: Complete**

Phase 3 is complete. The dashboard framework now has modular widgets, content-based change detection, durable state snapshots, skip-unchanged behavior, and refresh coalescing while preserving the verified full-frame partial/maintenance refresh paths.

## Phase 4 — Time and Wi-Fi status

**Status: Complete**

**Goal:** Add the first live system data without external content APIs.

Phase 4 checkpoints:

- [x] Phase 4A-1: Add a Wi-Fi connection manager with local credentials kept outside Git, connection-state reporting, and no additional network services (verified on hardware, 2026-10-03)
- [x] Phase 4A-2: Feed live Wi-Fi RSSI / connection state into the existing Wi-Fi widget through the Phase 3 coalescing path (verified on hardware, 2026-10-03)
- [x] Phase 4B-1: Add NTP-based time synchronization with explicit local timezone handling (verified on hardware, 2026-10-03)
- [x] Phase 4B-2: Feed minute-level live time into the Clock widget and verify coalesced clock + Wi-Fi updates (verified on hardware, 2026-10-03)

Phase 4 is complete. Live Wi-Fi status and NTP-synchronized Taiwan local time now flow through the existing Phase 3 coalescing and verified partial-refresh path.

**Next step: Phase 5 — introduce secure HTTPS market-data retrieval with certificate validation and a dedicated market-data service**

Planned work:

- Wi-Fi connection manager
- NTP time synchronization
- Local time handling
- Minute-level clock updates
- Wi-Fi RSSI measurement
- Upper-right Wi-Fi signal icon
- Reconnection behavior

## Phase 5 — Secure market data

**Status: Complete**

**Goal:** Display BTC, ETH, and HYPE perpetual-futures prices.

Phase 5A checkpoints:

- [x] Phase 5A-1: Add a minimal certificate-validating HTTPS GET to `fapi.binance.com`, gated on synchronized system time and reported through Serial only (verified on hardware, 2026-10-04).
- [x] Phase 5A-2: Add a dedicated BTC-only `MarketDataService` that parses/validates `symbol`, `price`, and optional `time`, and preserves the last valid value when a later request fails (verified on hardware, 2026-10-04).
- [x] Phase 5A-3: Extend `MarketDataService` to independent `BTCUSDT`, `ETHUSDT`, and `HYPEUSDT` last-valid slots and add a 60-second three-symbol polling flow. Results remain Serial-only (verified on hardware, 2026-10-04).
- [x] Phase 5B-1: Stage validated BTC/ETH/HYPE prices into the existing crypto widget states through `DashboardUpdateCoalescer`, while keeping the 60-second market polling cadence separate from the physical display refresh decision (verified on hardware, 2026-10-04).
- [x] Phase 5B-2: Explicitly verify unchanged-price skip behavior plus live market-data unavailability/recovery with Crypto widgets. Uses a one-shot application-level ETH unavailable simulation, while the real HTTP failure/last-valid path remains the Phase 5A-2 hardware-verified behavior (verified on hardware, 2026-10-04).
- [x] Phase 5B-3: Remove the one-shot Phase 5B-2 diagnostic injection from the production application path, restore normal 60-second BTC/ETH/HYPE polling only, and run a final Phase 5 regression before declaring Phase 5 complete (verified on hardware, 2026-10-04).

Planned work:

- Secure HTTPS client with certificate validation
- Dedicated market-data service
- BTC perpetual price
- ETH perpetual price
- HYPE perpetual price
- Failure handling that preserves the last valid value
- Independent data polling interval from display refresh interval

No production code should use `setInsecure()`.

Phase 5 is complete. Secure Binance USDⓈ-M market data, independent BTC/ETH/HYPE last-valid state, 60-second polling, live Crypto widget integration, unchanged-value skip behavior, short-failure hold/recovery behavior, and the cleaned production application path have all been verified on hardware. Prolonged outage/staleness UX remains deferred to Phase 9.

**Next step: Phase 7 — Physical button controls**

## Phase 6 — Weather

**Status: Complete (hardware verified 2026-10-05)**

**Goal:** Add weather information without coupling the weather provider to the UI.

Phase 6A checkpoints:

- [x] Phase 6A-0: Review the weather provider, current/hourly/daily data contract, local-coordinate privacy boundary, polling plan, and certificate-validation strategy. Open-Meteo + ISRG Root X1 selected as the implementation baseline; no firmware behavior changed in this design checkpoint (completed, 2026-10-04).
- [x] Phase 6A-1: Add the Open-Meteo trust anchor and perform a minimal certificate-validating HTTPS GET after Wi-Fi + NTP synchronization. HTTP 200, ISRG Root X1 validation, six hourly rows, and two daily rows verified on hardware; results remained Serial-only (verified on hardware, 2026-10-04).
- [x] Phase 6A-2: Add a dedicated `WeatherService` that validates and owns current conditions, a six-slot hourly strip (current hour + next five future hours), and today/tomorrow daily forecast data with last-valid preservation. Live parsing, midnight/date alignment, five future slots, daily values, and last-valid preservation after an induced invalid-coordinate failure were verified on hardware (2026-10-05).
- [x] Phase 6A-3: Add provider-code normalization / semantic weather mapping while preserving the raw WMO weather code for later UI refinement. All 29 documented WMO mappings plus UNKNOWN fallback, live semantic output, and last-valid preservation including normalized conditions were verified on hardware (2026-10-05).

Phase 6B checkpoints:

- [x] Phase 6B-1: Stage live current weather into the existing Weather widget through `DashboardUpdateCoalescer` without letting the service control display refresh. Live condition/temperature rendering, Weather widget mapping, and coalescing with simultaneous BTC/ETH/HYPE updates were verified on hardware (2026-10-05).
- [x] Phase 6B-2: Add the initial 30-minute weather polling flow and verify unchanged-value suppression plus unavailable/recovery behavior while retaining last-valid weather. Deterministic unchanged/failure/recovery diagnostics and a naturally scheduled 30-minute re-poll were verified on hardware (2026-10-05).
- [x] Phase 6B-3: Remove temporary diagnostics, run the final Phase 6 regression, document hardware results, and close Phase 6. Cleaned production weather polling, live Weather staging/coalescing, and regression behavior were verified on hardware (2026-10-05).

Planned work:

- Open-Meteo Forecast API
- Weather service abstraction
- Current temperature / conditions
- Six-slot hourly forecast strip: current hour + next five future hours
- Tomorrow high / low / representative condition / precipitation probability
- Weather icon mapping
- Initial 30-minute polling
- Graceful handling of unavailable network data
- Certificate-validating HTTPS with no `setInsecure()`

See [docs/PHASE6_WEATHER.md](docs/PHASE6_WEATHER.md) for the Phase 6A-0 provider, forecast-data, location, and TLS design review.

Phase 6 is complete. Weather transport, parsing, semantic normalization, current-widget integration, 30-minute polling, unchanged suppression, last-valid failure handling, and the cleaned production path have all been verified on hardware. Forecast-strip and tomorrow-summary presentation remain later UI work.

## Phase 7 — Physical button controls

**Goal:** Make the dashboard locally interactive.

Phase 7A checkpoints:

- [x] Phase 7A-1: Add a Serial-only hardware input probe for MENU (GPIO2), EXIT (GPIO1), rotary reference UP (GPIO6), and rotary reference DOWN (GPIO4), using active-low input handling and non-blocking debounce. Rotary CONF (GPIO5) remains intentionally unused. No input event triggers E-paper refresh in this checkpoint. Input detection and debounce behavior verified on hardware (2026-10-06).
- [x] Phase 7A-2: Separate Elecrow reference labels from EDP application semantics and adopt the logical mapping GPIO1=MENU, GPIO4=UP, GPIO6=DOWN, GPIO2=EXIT. Expose only logical `InputEvent` values to the application while preserving the verified active-low 30 ms debounce/release-event path and keeping GPIO5 unused. Serial-only logical mapping verified on hardware (2026-10-06).
- [x] Phase 7B-1: Add an application-owned `NavigationController` above the hardware input layer. Use three placeholder logical page slots only for state-machine verification: UP/DOWN cycle pages with wraparound in PAGE mode, MENU enters DETAIL for the selected page, EXIT returns to PAGE, and UP/DOWN inside DETAIL are intentionally deferred. Deterministic Serial self-checks and live state reporting verified on hardware; navigation remains independent of UI rendering and E-paper refresh (verified on hardware, 2026-10-06).
- [x] Phase 7B-2: Replace anonymous page slots with an application-owned `PageId` model and ordered top-level pages `DASHBOARD`, `WEATHER`, and `MARKETS`. `NavigationState` exposes the selected `PageId` while preserving PAGE/DETAIL behavior and wraparound. Named-page transitions, wraparound, and MENU/EXIT retention of the current PageId were verified on hardware. Page identity remains Serial-only and does not dispatch rendering or trigger E-paper refresh (verified on hardware, 2026-10-06).
- [x] Phase 7C-1: Connect top-level `PageId` changes to physical E-paper output using the existing verified full-frame partial-refresh sequence. Keep `DASHBOARD` on the production dashboard renderer; render minimal WEATHER and MARKETS placeholder pages for integration testing only. While a non-dashboard page is visible, continue staging background Clock/Wi-Fi/Market/Weather data but suppress dashboard dirty flushes so they cannot overwrite the selected page. When returning to DASHBOARD, render the newest pending dashboard state and commit it only after a successful physical refresh. MENU/EXIT mode-only changes remain non-rendering. Page cycling, reverse cycling, non-dashboard hold behavior, return-to-latest-dashboard behavior, and refresh quality were verified on hardware (2026-10-06).
- [x] Phase 7C-2: Replace the WEATHER placeholder with a functional weather page backed by the existing Phase 6 `WeatherService::lastValidSnapshot()` contract. Render current conditions, the five future hourly slots, and tomorrow summary with functional 1-bit typography only; keep MARKETS as a placeholder. If no last-valid weather exists, render a safe DATA NOT READY state. Track the last physically rendered weather snapshot and queue a WEATHER page refresh only when the visible snapshot actually changes, including hourly/tomorrow-only changes that may not dirty the compact Dashboard Weather widget. WeatherService remains data-only and never controls E-paper refresh directly. Live Weather page rendering, page switching, snapshot-change refresh gating, and refresh quality were verified on hardware (2026-10-07).
- [x] Phase 7C-3: Replace the MARKETS placeholder with a functional market page backed by Phase 5 BTCUSDT / ETHUSDT / HYPEUSDT last-valid values. Keep each symbol independent: unavailable symbols render `--` while valid symbols keep their last-valid price. Render only provider-neutral BTC / ETH / HYPE labels and visible price strings; Binance source-time metadata is intentionally not part of the page dirty contract. Track the last physically rendered Market page state and queue a MARKETS refresh only when at least one visible price string changes. MarketDataService remains data-only and never controls E-paper refresh directly. Live market rendering, in-place price refresh, page switching, return-to-dashboard behavior, and refresh quality were verified on hardware (2026-10-07).


Phase 7D checkpoints (hardware verified on the development panel, 2026-10-08):

- [x] Phase 7D-1: Make Dashboard MENU show a real two-option DETAIL page with STANDBY and DISPLAY CLEAN. UP/DOWN switch selection, MENU confirms, EXIT returns to the newest Dashboard. Application suppresses background Dashboard flushes while DETAIL is visible. Preserve existing WEATHER / MARKETS navigation and refresh paths. Dashboard DETAIL navigation and return behavior verified on hardware (2026-10-08).
- [x] Phase 7D-2: STANDBY draws a small 1-bit crescent moon, performs one verified partial page update, puts the E-paper controller to sleep, disables GPIO7 panel power with deep-sleep GPIO hold, disables Wi-Fi, and enters ESP32-S3 Deep Sleep. EXT1 ANY_LOW wake is restricted to GPIO1=MENU or GPIO2=EXIT; waking restarts production firmware. Moon display, standby entry, selected-key wake, and normal startup resumption verified functionally on hardware (2026-10-08). GPIO7 rail voltage and full-board sleep current were not electrically measured.
- [x] Phase 7D-3: DISPLAY CLEAN recomposes the latest Dashboard and performs one user-requested, Phase 1F-verified maintenanceRefresh (fast clear → physical white → previous RAM white → new frame → partial refresh → RAM sync), not raw Full Refresh. Commit pending dashboard state only after a successful refresh; later partial updates remained crisp in hardware testing (2026-10-08).

See [docs/PHASE7_SYSTEM_ACTIONS.md](docs/PHASE7_SYSTEM_ACTIONS.md) for the safety boundaries and practical verification checklist.

Phase 7 accepted scope is complete as of 2026-10-08: the hardware buttons, debounced short-press navigation, three real pages, Dashboard DETAIL menu, moon STANDBY/Deep Sleep wake, and on-demand DISPLAY CLEAN are hardware-verified.

Scope decisions (2026-10-08):

- **Manual data refresh: deferred, not a Phase 7 acceptance requirement.** The existing minute-level market/clock and 30-minute weather polling is sufficient for current use. For an exceptional malfunction the user prefers the device's hardware Reset/restart over an extra UI command; reset is not being treated as a substitute for DISPLAY CLEAN.
- **Deep Sleep current / power consumption measurement: explicitly out of scope for this USB-powered, no-battery configuration.** Standby/wake functionality is verified, but power rail voltage and sleep current are not measured and must not be presented as numerical evidence of low power. No measurement is required to close Phase 7.
- Long-press / double-press and extended settings/network controls remain optional future ideas, not blockers for Phase 8.

## Phase 8 — UI refinement

**Status: In progress — Phase 8A static Dashboard and Phase 8B-2/8B-3/8B-4 isolated live NTP, perpetual markets and current-weather previews have been tested successfully on CrowPanel (2026-10-11). Phase 8B-5 Wi-Fi/production routing, six-day forecast and full regression remain pending.**

**Goal:** Improve readability and visual quality without sacrificing E-paper stability.

Phase 8A-1 checkpoints:

- [x] Build an opt-in static 792×272 dashboard proof of layout, using the user's PDF: weather/date-time/Wi-Fi, ETH/BTC/HYPE, and six mock daily weather slots. Use exploratory tabular numeric bitmap glyphs, detailed 1-bit logo samples, sparkle separators and diamond-ended dividing lines.
- [x] Keep preview off by default and isolated from all production data/polling; use the existing verified maintenance refresh once when explicitly enabled through ignored local config.h.
- [x] Initial static prototype was compiled/uploaded and functionally verified on-panel by the user (2026-10-10).
- [x] Update the static prototype to match the approved revised reference: small outline sun in both header and 6-day strip, 50%-height centered date/weekday, diamond separators, 30px outline crypto icons without middle sparkles, and six single-line summaries.
- [ ] Recompile/reupload **revised** artwork and verify final layout on hardware, including x=396 seam, pixel clipping, icon clarity and refresh artifacts.
- [ ] Capture user feedback, refine assets and record hardware results before integrating live dashboard data.

Phase 8A-1 Rev-B (2026-10-10): user hardware feedback confirms the second static prototype exposes broken calendar glyphs, unbalanced spacing, low-quality logos and coarse large numerals.

- [x] **Rev-B Step 1 — Font & Logo Assets (source committed, software data checks passed):** repair 17px date/day glyph atlas, use native 34px non-overlapping numeric glyphs, refine 11px forecast numerals, and manually redraw native 30px 1-bit ETH/BTC/HYPE outlines. `main` and production data/refresh code are untouched.
- [x] Rev-B Step 1 compiled/uploaded and checked on-panel by the user (2026-10-10 photo): repaired text looks improved, but uneven font size/weight remains; visual acceptance is **not** yet complete.
- [x] Rev-B typography correction: unify native bitmap style to 34px numbers, 17px header/captions/weekday, and 13px weather numerals. No changes to layout or production drivers.
- [ ] Recompile/reflash revised typography and confirm consistent visual weight and readability in the next photo.
- [ ] **Rev-B Step 2 — Layout (deferred until Rev-C fonts are accepted):** upper divider y=81 → y=77; rebalance three market groups and center all six weather columns with uniform spacing.
- [ ] **Rev-B Step 3 — Hardware acceptance:** verify readable glyphs/logos, absence of overlap/clipping, seam quality and partial refresh stability before live-data integration.

Phase 8A-1 Rev-C — Font Reset (new independent native bitmap family):

- [x] **Rev-C1:** Specify stable fixed-cell fonts (34px 22×34, 17px 12×17, 13px 8×13), exact glyph coverage, identical advance per font, and design/maintenance contract.
- [x] **Rev-C2:** Create readable `fonts_src/DashboardFont*.glyphs` and generated `DashboardFont*.h` files plus `tools/generate_dashboard_fonts.py`. Readback/static re-encoding of all 75 glyphs passed.
- [x] **Rev-C3 implementation:** Add four dedicated font-only pages with opt-in local config and Serial Monitor page selection; use only the existing verified maintenance refresh, preserving all production paths. Static sample glyph/bounds checks passed.
- [x] **Rev-C3-JB1 font source adoption (code committed, hardware pending):** Replace isolated Rev-C3 hand-drawn raster glyphs with official JetBrains Mono Medium-derived 34/17/13px 1-bit glyphs. Preserve all four test pages and fixed-cell metrics; no Rev-C4 layout integration.
- [x] **Rev-C3-JB1 hardware typography feedback:** User finds the JetBrains Mono 34px and 17px samples excellent; 13px appears too small (2026-10-10). This does not close all Phase 8 acceptance criteria.
- [x] **Rev-C3-JB2 implementation (hardware pending):** Add independently rasterized 9×14px JetBrains Mono Medium (17 glyphs) alongside unchanged 13/17/34px assets; extend isolated Serial diagnostics to five pages (4=14px, 5=13-vs-14). Update generator and docs; production rendering remains unchanged.
- [x] **Rev-C3-JB2 typography approval (hardware, 2026-10-10):** User reports satisfactory actual-panel results and selects 34/17/14px JetBrains Mono Medium; 13px is superseded for Dashboard use. This accepts font size/appearance, not the full Dashboard layout.
- [x] **Rev-C4 source integration:** Switch only `Phase8A1Preview.cpp` to `DashboardFont34/17/14`, scale=1. Leave all font pixel data, divider/layout positions, icons, services and E-paper driver unchanged; source-level fixture bounds validated.
- [x] **Rev-C4 hardware visual acceptance (2026-10-10):** User uploaded/tested the integrated 34/17/14px JetBrains Mono static Dashboard and reported the result acceptable. This approves the Rev-C4 visual baseline, **not** future layout edits or live-data integration.
- [x] **Rev-D1 layout code committed (device validation pending):** Move top divider y=81 → y=77 and truly center all six 128px forecast columns across 792px, placing each icon+17px weekday unit and 14px summary by measured bitmap widths. All other positions, fonts, icons, refresh, services and production firmware untouched. Geometry check PASS.
- [x] **Rev-D1 on-panel display check (2026-10-10):** User reports the revised static Dashboard displays normally on hardware. The user requested one follow-up visual spacing adjustment in the six-day forecast summary before closing Rev-D1 completely.
- [x] **Rev-D1a text-spacing adjustment (source committed):** All six mock summaries changed from `25-29 / 10 %` to `25-29/ 10%`, retaining the original 14px font and dynamic centering in 128px columns. No other layout or hardware behavior was changed.
- [x] **Rev-D1a hardware acceptance (2026-10-10):** User confirms the updated `25-29/ 10%` text appears OK on the physical E-paper panel.
- [x] **Rev-D2 implementation (hardware pending):** Remove the middle-region ETH/BTC/HYPE logos and `USDT PERP` footers in the opt-in static Dashboard preview. Show 17px ETH/BTC/HYPE above their corresponding 34px prices, with independent centering in three equal 248px columns and no changes to fonts, dividers, header, bottom forecast or display drivers.
- [x] **Rev-D2 on-panel visual acceptance (2026-10-11):** User reports the revised ETH/BTC/HYPE text-only market row displays correctly and is satisfactory on the CrowPanel. The 17px symbol-above-34px-price arrangement is accepted as the new static-preview baseline. This feedback confirms visual appearance, not extended refresh or live-data behavior.
- [x] **Rev-D2 static-layout handoff:** Preserve the accepted Rev-D2 appearance as the fixed reference for icon and later live-data work. No production rendering changes.

Phase 8A-2 — Weather Icon System (feature branch; complete for isolated static visual design and device acceptance on 2026-10-11):

- [x] **8A-2A classification contract:** Define eleven 32×32 native monochrome icons for clear day/night, partly cloudy day/night, overcast, fog, drizzle, rain, snow, thunder and unknown; map existing 16 `WeatherCondition` categories, with day/night only where meaningful.
- [x] **8A-2B source artwork and reproducibility:** Add editable `weather_icons_src/WeatherIcons32.icons` and 1-bit packed `WeatherIconAssets.h`, plus `tools/generate_weather_icons.py`; source/header bitwise readback check passed for all 11 × 128-byte assets.
- [x] **8A-2B gallery and isolated selector:** Add `WeatherIconSelector` and a 4-column × 3-row Gallery with 11 native 32×32 samples and 17px labels. Use opt-in local `EDP_PHASE8_WEATHER_ICON_TEST=1`, mutually exclusive with font test and static preview. Bounds and 16-category mapping review passed; no production-service or refresh-path edits.
- [x] **8A-2A first Gallery on-panel inspection (2026-10-11):** User confirms all eleven test icons display normally on CrowPanel, but requests the original white-on-black thin-outline reference artwork style instead of our first draft. This is an acceptance of Gallery functionality, **not** visual approval of the first icons.
- [x] **8A-2 Rev-B source artwork:** Recreate all eleven 32×32 editable monochrome glyphs using the user-supplied icon reference. Preserve the small ~21–23px optical footprint centered on 32×32, fine outline style, exact category names and existing test-only Gallery/condition selector. Source-to-generated-header independent byte-level readback PASS. No forecast/wind additions; no service/driver/production Dashboard changes.
- [x] **8A-2 Rev-B on-panel visual approval (2026-10-11):** User confirms the reference-style weather icon Gallery displays correctly and all eleven icon appearances are acceptable on the real CrowPanel. This verifies visual output, not future dynamic icon switching, long-duration refresh, or production integration.
- [x] **8A-2D implementation (code committed, device acceptance pending):** Replace the handcrafted outline sun in `Phase8A1Preview.cpp` with `WeatherIconSelector::select()` and approved 32×32 Rev-B assets. Header uses CLEAR day; MON…SAT respectively show CLEAR, PARTLY_CLOUDY, OVERCAST, DRIZZLE, RAIN, THUNDERSTORM daytime representative icons. All are static fixtures; existing 34/17/14px typography, 32×32 icon slot coordinates, dividers, markets, Wi-Fi, six-day formatting, E-paper driver and production state/refresh remain untouched. Source bounds checks passed.
- [x] **8A-2D integrated Dashboard real-panel acceptance (2026-10-11):** User reports the complete static Dashboard with approved header + six forecast weather icons displays normally on the CrowPanel. Accept the integrated Phase 8A appearance as the next development baseline; this is a one-shot integrated visual/functionality check, not a long-duration refresh or live-data test.

**Phase 8A visual-design closeout (2026-10-11):** The final opt-in 792×272 preview now has accepted JetBrains Mono Medium 34/17/14px text, the Rev-D2 three-market text-only layout, centered six-slot forecast row, and the user-approved 11-weather-icon family. Rev-B Gallery and Rev-D2/8A-2D combined-page display have been tested on hardware. Keep the feature-branch renderer and its assets stable as the Phase 8B visual reference; do not treat Phase 8A acceptance as evidence that the live Dashboard has been integrated or verified.
 
Phase 8B — Live Dashboard integration (isolated feature branch):

- [x] **8B-1 interface audit and design (2026-10-11):** Read current `TimeService`, `MarketDataService`, `WeatherService`, `WiFiManager`, `DashboardState`, `DashboardStateSnapshot`, dirty/coalescer, `Dashboard::render()`, `PageRenderer`, Phase 8A static preview and all application display paths. Identify existing reusable data and state contracts, missing calendar and six-day daily data, dynamic-price width constraints and integration risks. Record a bounded integration contract in `docs/PHASE8_LIVE_INTEGRATION.md`. No firmware behavior changed.
- [x] **8B-2 code implementation (hardware pending, 2026-10-11):** Introduce bounded, owned `Phase8CalendarHeader` (`HH:MM`, `DD MMM`, weekday) plus locale-independent `Phase8CalendarPresentation` converter/self-check. Refactor only the accepted Phase 8A header renderer to take presentation input; retain immutable `Phase8A1Preview::render()` fixture for regression. Add mutually exclusive `EDP_PHASE8B2_CLOCK_TEST=1` mode using **existing** `WiFiManager` and `TimeService` NTP with `CST-8`. WAITING baseline via verified maintenance refresh, then partial refresh only on changed time/date/weekday after NTP sync. On refresh failure, preserve last displayed buffer/state for retry. Markets, weather, six-day forecast and Wi-Fi icon are still explicitly **MOCK** in this mode. Static source/readback checks and calendar-only C++11 host equivalent self-check PASS; full Arduino compile and panel behavior unverified.
- [x] **8B-2 hardware acceptance (user report, 2026-10-11):** User uploaded and tested the isolated `EDP_PHASE8B2_CLOCK_TEST` firmware and reports the actual CrowPanel test is **normal**. This accepts the tested live Taiwan NTP time/date/weekday preview and its observed refresh behavior as the 8B-2 device gate. Do not infer a physical midnight rollover, Wi-Fi disconnect/reconnect stress scenario, or long-duration ghosting assessment; formatter boundary cases were source/self-check tests, not manually observed on the device. Proceed to 8B-3 price slice.
- [x] **8B-3 first code checkpoint (2026-10-11; hardware pending):** Add bounded owned `Phase8MarketPrices` and decimal-string formatter with symbol-specific 1/2/3 fractional digits (BTC/ETH/HYPE), half-up carry, ≤10 glyph/220px guard, deterministic precision reduction and explicit `--` on unrenderable input. Keep the original raw Binance price unchanged in `MarketDataService`; use its TLS-verified three-symbol USDⓈ-M futures polling. Extend the existing Phase 8A shared composition with `renderWithCalendarAndMarkets()` while retaining 8A and 8B-2 fixture entry points. Add opt-in, mutually exclusive `EDP_PHASE8B3_MARKET_TEST`: NTP calendar + 3 prices live; static weather/forecast/Wi-Fi art; poll 60 sec and coalesce changes to one existing application-owned previous-RAM/partial refresh. Host C++11 formatter mirror self-check and GitHub static review PASS; Arduino compile and physical panel approval pending.
- [x] **8B-3 on-device acceptance (user report, 2026-10-11):** User reports **「實際測試正常」** after running the opt-in NTP + real BTCUSDT/ETHUSDT/HYPEUSDT market-price Dashboard on the physical CrowPanel. Accept Phase 8B-3 integrated visual/functionality test as the next working baseline. This report does **not** separately establish 60-second cadence timing logs, repeated unchanged-data suppression, deliberate API/Wi-Fi failures, last-valid recovery under failure, extreme-value formatter stress on-device, prolonged ghosting/endurance or an observed midnight transition. Keep those regression checks planned for 8B-6.
- [x] **8B-4 isolated code implementation (2026-10-11; device test pending):** Add `Phase8CurrentWeather` owned presentation with approved 32px icon pointer and bounded 17px short label / 14px one-decimal temperature; map all 16 `WeatherCondition` categories and day/night via `WeatherIconSelector`. Retain all Phase 8A, 8B-2 and 8B-3 rendering fixtures through the same shared `Phase8A1Preview` composition. Add mutually exclusive `EDP_PHASE8B4_WEATHER_TEST`: live NTP, BTC/ETH/HYPE 60-second polls and Open-Meteo current condition/temperature/isDay 30-minute polls with verified pre-existing TLS. Startup uses UNKNOWN icon + NO DATA placeholder, successful fetch stages last-valid, failures preserve visible weather, and selected bitmap pointer participates in visible-state comparison. Application coalesces changed fields into one existing previous-RAM/partial-refresh cycle. No production WeatherService, PageRenderer, coalescer or driver changes. GitHub source/readback checks and isolated host C++11 weather formatter/self-check PASS. Full Arduino compile and hardware validation pending.
- [x] **8B-4 real CrowPanel acceptance (user report, 2026-10-11):** User reports **「實際測試正常」** after testing the isolated live NTP + ETH/BTC/HYPE + Open-Meteo current-weather Dashboard on the physical panel. Accept Phase 8B-4's normal operation and integrated visual rendering as the next working baseline. This general report does **not** separately verify an observed 30-minute weather repoll, clear day/night bitmap transition, forced Wi-Fi/API failures or last-valid recovery, midnight rollover, unchanged-state suppression logs, or long-running ghosting tests. Preserve those checks for Phase 8B-6; Phase 8B-5 production/page routing remains pending.
- [x] **8B-5A isolated Wi-Fi icon implementation (2026-10-11, hardware pending):** Preserve the accepted 8B-4 live NTP, three Binance USDⓈ-M perpetual markets, Open-Meteo current weather and all fixed UI geometry. Reuse existing production `wifiIconForRssi()` thresholds and five `Icons::WIFI_*` 32×24 bitmaps, with `WiFiManager` as the connection/RSSI owner. Add `renderWithCalendarMarketsWeatherWifi()` to the SAME graphics compositor; leave 8A, 8B-2, 8B-3, 8B-4 rendering APIs and mock-strong fixture unchanged. Add a mutually exclusive `EDP_PHASE8B5A_WIFI_TEST` mode: disconnected baseline, immediate link-state changes, 60-second RSSI sampling, selected-bitmap-pointer visible comparison, one combined application-owned partial refresh, last displayed state committed only after successful physical update. No modifications to weather/market services, TLS, driver, normal Dashboard/PageRenderer, buttons or deep sleep.
- [x] **8B-5A on-panel acceptance (user report, 2026-10-11):** User reports **「實際測試正常」** after testing the opt-in `EDP_PHASE8B5A_WIFI_TEST` mode on the physical CrowPanel. Accept the dynamic Wi-Fi icon combined with live NTP, three perpetual prices and current weather as the next normal-operation baseline. This general report does **not** independently verify deliberate Wi-Fi off/on fault injection, every RSSI boundary level, a same-grade/no-refresh trace, physical midnight rollover, error recovery or extended ghosting; reserve these for 8B-6. Phase 8B-5B normal application/page integration remains **not implemented**.
- [x] **8B-5B opt-in normal-application code integration (2026-10-11; physical verification pending):** Introduce owned `Phase8DashboardFrame` with explicit latest/displayed states, per-rendered-field equality (calendar incl. weekday, prices as 34px rounded strings, day/night weather bitmap, RSSI selected bitmap), stage after the **existing** production NTP/market/weather/Wi-Fi service paths, commit only after a successful physical update. Expose the same validated Phase 8A composition as `Dashboard::renderLive()` and optional `PageRenderer::useLiveDashboardFrame()`; route application startup baseline, partial/maintenance, Dashboard page return and Phase 7 DETAIL `DISPLAY CLEAN` via the same live frame, with WEATHER/MARKETS and standby/deep sleep unchanged. Keep old application when the **exclusive** `EDP_PHASE8B5B_APP_TEST=1` flag is off; do not merge into `main` yet. Replace the six entirely fictional daily mock cells with an explicit `6-DAY FORECAST UNAVAILABLE` caption in this live route only. Modern startup uses already accepted `maintenanceRefresh()`; no extra full-refresh cycle, no driver or TLS changes. GitHub source/readback checks PASS; **Arduino build and device tests not yet completed**.
- [x] **8B-5B physical acceptance (user report, 2026-10-11):** Following the opt-in `EDP_PHASE8B5B_APP_TEST=1` compile/upload and physical-device check request, the user reports **「實際測試正常」**. Accept the **normal observed CrowPanel operation** of the new live Dashboard integrated with existing Phase 7 application/page/DETAIL/standby flows as the working 8B-5B baseline. In this mode, six imaginary daily forecast rows are replaced with an explicit unavailable notice. This broad positive report does **not** individually establish fault-injection, all menu branches, observed midnight rollover, repeated unchanged-state suppression with logs, every RSSI level, long-duration ghosting, or weather repoll boundary; keep those for targeted 8B-6 regression. This does not merge the feature branch into `main`. See `docs/PHASE8B5B_PRODUCTION.md`.
- [x] **8B-6A source-level regression audit and physical test protocol (2026-10-11):** Read latest accepted 8B-5B code at `546c9de0697ebb656a2cbcda34bb5c08cb1165b1`: `CrowEPD579::maintenanceRefresh` and `restoreFrameStateForPartial` sequence unchanged; owned `Phase8DashboardVisualState` calendar/weather/day-night/RSSI/rounded-price visible diff with success-only commit present; only app owns physical refresh; Phase 7 PageRenderer, DETAIL, DISPLAY CLEAN, STANDBY and wake entry points still connected; TLS CA validation retained; live mode explicitly shows `6-DAY FORECAST UNAVAILABLE`. Document separate device checks in `docs/PHASE8B6_REGRESSION.md`. **This is a source audit, not an ESP32 rebuild or extended hardware stress-test. No firmware changed.**
- [x] **8B-6A short functional device regression (user report, 2026-10-11):** User confirms **all three explicitly requested on-panel tasks are normal**: (1) UP/DOWN navigation among Dashboard, Weather and Markets; (2) Dashboard DETAIL → DISPLAY CLEAN and three subsequent minute-level partial refreshes without reported blurring/ghosting; (3) STANDBY moon/deep sleep, wake by MENU/EXIT and return to the Dashboard. These map to A2, A4, A5, A6 and basic DETAIL interaction. This is real hardware acceptance of the **8B-6A short-test gate**. The user did not separately report leaving DETAIL open for 1–2 minutes, explicit startup A1 log, or detailed per-action Serial output; do not claim them individually verified. No firmware source change.
- [x] **8B-6B network and visible-diff device regression (user report, 2026-10-11):** After the three explicit 8B-6B device tasks, the user reports **「實際測試三項都正常」**: (1) Wi-Fi disconnection/last-valid values and clock behavior, (2) Wi-Fi recovery and resumed live data, (3) market/weather polling and visible-diff/Dirty Tracking behavior. Accept the **8B-6B three-task normal-operation hardware gate PASS**. The user did not provide individual serial traces, RSSI boundary-by-boundary measurements or evidence proving the exact 30-minute poll timing/no-change suppression mechanism; avoid claiming instrumented proof for these subcases. Phase 8B-6C endurance/cross-midnight remains pending. No firmware or network/TLS settings changed.
- [x] **8B-6 scope closed for current four live categories (2026-10-11):** 8B-6A (page/clean/standby) and 8B-6B (Wi-Fi/data recovery/visible-change tracking) passed physical tests per user reports. **Long-duration E-paper endurance and naturally observed midnight/day-night transitions are explicitly DEFERRED**, not marked PASS; do not delay development of the missing six real daily forecast rows for incomplete-Dashboard endurance testing.
- [x] **Phase 8 final endurance gate scheduled (deferred by user, 2026-10-11):** After Phase 8C live six-day forecast, final UI/page integration and short regressions are accepted, run 1–2-hour (and optional overnight) full-feature panel stability across minute partials, weather/market/RSSI/forecast changes, controller center seam, maintenance-aftercare, date change and ghosting. Record real observed outcomes then; no early PASS.
- [x] **8C-1 interface/contract review:** Official Open-Meteo `/v1/forecast` uses `forecast_days=7` to request local today + six future dates, keeping `daily=weather_code,temperature_2m_max,temperature_2m_min,precipitation_probability_max` and `timezone=Asia%2FTaipei`. Current `WeatherService` requests only 2 days and exposes `today`/`tomorrow`. Plan an additive `fetchLatestSixDay()` path with 7 validated dates while retaining the old two-day method and Weather page contract. Isolated serial-only physical data acceptance precedes any lower-strip/UI changes.
- [x] **8C-1 live data source / parser / serial diagnostic code (2026-10-11; hardware pending):** Extend `WeatherSnapshot` with six owned `futureDays` and `hasSixDayForecast`, and add opt-in `WeatherService::fetchLatestSixDay()` which requests `forecast_days=7` (today+6) via the existing secure HTTPS client. Strictly parse all 7 daily field arrays, check finite bounded max/min and probability/WMO, date alignment to current local day, 7 consecutive Gregorian dates (leap/month/year boundary) and last-valid atomic commit. Keep legacy `fetchLatest()` at `forecast_days=2` with its original public `today`/`tomorrow` shape and unchanged app call sites. Exclusive serial-only `EDP_PHASE8C1_DAILY_TEST` prints real six future rows; does not perform E-paper refresh. Static/preprocessor/call-site checks PASS; ESP32 compile/HTTPS hardware outcome **not verified**.
- [x] **8C-1 real-device acceptance (user report, 2026-10-11):** User replied **「實際測試正常」** to the opt-in `EDP_PHASE8C1_DAILY_TEST=1` ESP32-S3 Serial-only test. Accept the **normal physical operation of the real Open-Meteo today + six future daily-data retrieval/validation gate** and proceed to 8C-2 presenter/graphics. This report establishes overall test PASS but did not include verbatim six daily rows, individual TLS traces, a screenshot or a fully reproduced Serial transcript, so do not claim those were independently inspected. Original 2-day production WeatherService call and the E-paper driver remain unchanged; this sign-off changes docs only.
- [x] **8C-2 six-column presenter/renderer and isolated hardware test mode implementation (2026-10-11; hardware pending):** Add `Phase8SixDayPresentation` as a bounded, owned rendering contract for six real `WeatherSnapshot::futureDays[6]` rows, with Gregorian weekday mapping from local YYYY-MM-DD, 32×32 approved daytime representative condition assets, a native 14px summary `MAX/MINC PRECIP%` (e.g. `29/25C 80%`) and date/temperature/precip/consecutive-day validation. Preserve the accepted six 128px centered Rev-D2 columns (x=12 base, icon y=200, 17px weekday y=209, 14px summary y=243), header/market drawing and old static demo/8B live unavailable entry points. Add `Phase8A1Preview::renderWithSixDayForecast()` sharing the same composition, and exclusive `EDP_PHASE8C2_DISPLAY_TEST`: fetch real Open-Meteo seven-date response via verified TLS, render exactly once with `maintenanceRefresh()` and log displayed text; no Binance/NTP/poll/page actions in this isolated mode. GitHub source checks PASS; **Arduino compilation and actual CrowPanel typography/fit are pending.**
- [x] **8C-2 on-device graphics acceptance (user report, 2026-10-11):** After the exclusive `EDP_PHASE8C2_DISPLAY_TEST=1` six-real-future-day graphics procedure, user reports **「實際測試正常」**. Accept the **normal on-panel six-column forecast rendering** of approved 32×32 condition icons, 17px weekdays and 14px max/min°C + precipitation summaries as the Phase 8C-2 hardware gate. This is a general user acceptance report: no separate photograph, per-row data, compile transcript or exact Serial output was supplied, so do **not** claim those artifacts were individually examined or that all atypical temperature widths were physically tested. The isolated upper WAITING/-- fields remain expected. **8C-3 production app dirty/30-minute forecast integration has not been implemented.** No firmware changed by this sign-off.
- [x] **8C-3 opt-in integrated live Dashboard code (2026-10-11; real-device acceptance PENDING):** Under `EDP_PHASE8B5B_APP_TEST=1` **and** `EDP_PHASE8C3_APP_TEST=1`, switch the existing single 30-minute production `WeatherService` fetch to `fetchLatestSixDay()` (today + six genuine future days), preserving current/hourly/tomorrow Weather page fields and the same verified CA client. Extend owned `Phase8DashboardFrame` with `Phase8SixDayForecast`: `setUnavailable()`, visible `equal()`, stage, pending and commit-on-physical-success through the existing `Phase8DashboardVisualState`. Production `Dashboard::renderLive()` reuses the 8C-2 accepted six-column compositor. App staging preserves last visible forecast on HTTPS failures or presenter-overflow, coalesces daily icon/weekday/rounded max-min°C/precip changes with NTP/prices/Wi-Fi and renders the same latest state after page return or Phase 7 DETAIL DISPLAY CLEAN. Keep 8B-5B alone and older mock previews available with old behavior; no extra network/refresh scheduler, full refresh, modified driver or TLS exception. GitHub code/source readback PASS; **Arduino compile, long/poll cadence and hardware integration tests NOT YET PASSED**.
- [x] **8C-3 normal app on-panel acceptance (user report, 2026-10-11):** Following the three specifically requested tests with both `EDP_PHASE8B5B_APP_TEST=1` and `EDP_PHASE8C3_APP_TEST=1`, user confirms **「實際測試三項都正常」**: (1) complete live Dashboard showing NTP/calendar, three Binance perpetual prices, current Open-Meteo, Wi-Fi RSSI and **six real future-day forecasts**; (2) ordinary minute-level/30-minute weather poll plus app-owned coalesced partial behavior; (3) Dashboard/Weather/Markets navigation and Phase 7 DETAIL/DISPLAY CLEAN/standby wake behavior. **8C-3 normal hardware integration PASS** as reported. No per-event Serial log, exact 30-minute timing trace, individual fault-injection case or multi-hour endurance evidence was supplied; those are not individually certified. Feature branch `main` promotion and the user-deferred Phase 8 final full-feature stability gate remain pending. Documentation-only acceptance; no firmware changes.
- [x] **Enterprise Wi-Fi PEAP implementation staged (2026-10-12; compile/on-device verification PENDING):** On `phase8a1-static-prototype`, add optional WPA2-Enterprise EAP-PEAP authentication through existing nonblocking `WiFiManager`, retaining default Personal Wi-Fi and all Phase 8 modes. Require CA + expected RADIUS DNS name and check ESP-IDF EAP configuration return codes without insecure fallback. Office-specific SSID, identity, password and public trusted CA stay in ignored local `config.h`; no display/driver/HTTPS data pipeline changes. Mac logs independently confirm EAP-PEAP (25); inner EAP method, live RADIUS certificate identity and actual ESP32 connection remain to be verified. **DO NOT mark as hardware PASS before compile, accepted EAP security validation and IP/NTP/HTTPS regression.** See [Enterprise Wi-Fi implementation and checks](docs/ENTERPRISE_WIFI.md).
- [ ] **Phase 8 final full-feature endurance gate (after 8C-3 normal integration PASS):** Per user decision, test 1–2h and optional overnight when all Dashboard fields are truly live; verify dual SSD1683 seam, maintenance-aftercare, consecutive partial sharpness, cross-day weekday and weather day/night/icon/forecast date shifts, ghosting and refresh suppression. No acceptance claim until physical observations.

**8B integration guardrails:** Existing production `Dashboard::render()` still draws the old Phase 7 dashboard; Phase 8A uses an opt-in mock renderer. No live production wiring has been accepted as of 8B-1. The calendar/day forecast are absent from the present `DashboardState` dirty/snapshot ownership; an independent visual update cannot silently bypass that mechanism. `DashboardWidgetState` text values must outlive rendering and fit fixed capacities. Audit all display entry points before switching the renderer: baseline, partial/maintenance flush, page transition, DETAIL DISPLAY CLEAN. No change to the SSD1683 maintenance sequence, TLS validation or system services.

See [Phase 8B-1 interface audit and integration contract](docs/PHASE8_LIVE_INTEGRATION.md) for exact types, risks, sequencing and acceptance tests.

See [Phase 8 weather icon notes](docs/PHASE8_WEATHER_ICONS.md) for mapping and Gallery test instructions. Phase 8B live-data integration remains pending.

See [Rev-C font design notes](docs/PHASE8_FONT_REVC.md) and [Phase 8 UI prototype notes](docs/PHASE8_UI.md). Six future daily forecast values are static mock data in 8A-1; actual 6-day WeatherService expansion is later work.

Planned work:

- Refine fonts and spacing
- Crypto logos
- Weather icons
- Wi-Fi icon polish
- Optional 1-bit dithering where it remains legible
- Multiple dashboard pages if useful
- Refresh-frequency tuning

## Phase 9 — Reliability and release hardening

**Goal:** Turn the prototype into a dependable always-on device.

Planned work:

- Long-duration refresh testing
- Wi-Fi outage/recovery tests
- API failure tests
- Memory/heap monitoring
- Watchdog strategy if needed
- Power behavior review
- Verify no unintended network services
- Verify no secrets are committed
- Tag the first stable firmware release

## Refresh-frequency guideline

Different information should use different data refresh cadences:

| Data | Initial target cadence |
| --- | --- |
| Clock | 1 minute |
| BTC / ETH / HYPE | 1–5 minutes |
| Wi-Fi signal | 1–5 minutes |
| Weather | 15–60 minutes |

Data polling and physical E-paper refresh should remain separate concepts. Multiple changes should be combined into one refresh whenever practical.
