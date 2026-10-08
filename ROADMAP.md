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

Planned work:

- Identify and verify panel button GPIOs
- Debouncing
- Short press
- Long press
- Double press if reliable
- Page switching
- Manual data refresh
- Status/settings view
- Optional controlled network/reset actions

## Phase 8 — UI refinement

**Goal:** Improve readability and visual quality without sacrificing E-paper stability.

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
