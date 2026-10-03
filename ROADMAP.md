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

**Status: In Progress**

**Goal:** Display BTC, ETH, and HYPE perpetual-futures prices.

Phase 5A checkpoints:

- [x] Phase 5A-1: Add a minimal certificate-validating HTTPS GET to `fapi.binance.com`, gated on synchronized system time and reported through Serial only (verified on hardware, 2026-10-04).
- [x] Phase 5A-2: Add a dedicated BTC-only `MarketDataService` that parses/validates `symbol`, `price`, and optional `time`, and preserves the last valid value when a later request fails (verified on hardware, 2026-10-04).
- [x] Phase 5A-3: Extend `MarketDataService` to independent `BTCUSDT`, `ETHUSDT`, and `HYPEUSDT` last-valid slots and add a 60-second three-symbol polling flow. Results remain Serial-only (verified on hardware, 2026-10-04).
- [x] Phase 5B-1: Stage validated BTC/ETH/HYPE prices into the existing crypto widget states through `DashboardUpdateCoalescer`, while keeping the 60-second market polling cadence separate from the physical display refresh decision (verified on hardware, 2026-10-04).
- [ ] Phase 5B-2: Explicitly verify unchanged-price skip behavior plus market fetch failure/recovery with live Crypto widgets, and decide whether stale-data/status indication is required before closing Phase 5.

Planned work:

- Secure HTTPS client with certificate validation
- Dedicated market-data service
- BTC perpetual price
- ETH perpetual price
- HYPE perpetual price
- Failure handling that preserves the last valid value
- Independent data polling interval from display refresh interval

No production code should use `setInsecure()`.

## Phase 6 — Weather

**Goal:** Add weather information without coupling the weather provider to the UI.

Planned work:

- Select a weather data provider
- Weather service abstraction
- Temperature / conditions
- Weather icon mapping
- Sensible low-frequency polling
- Graceful handling of unavailable network data

## Phase 7 — Physical button controls

**Goal:** Make the dashboard locally interactive.

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
