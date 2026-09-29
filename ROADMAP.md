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

**Status: In progress**

**Current step: Phase 2A complete — reusable primitives and visible-coordinate mapping are hardware-verified**

**Goal:** Build our own compact black/white rendering layer.

Planned work:

- Pixel, line, rectangle primitives
- Text rendering
- Custom font support
- Bitmap/icon support
- Small icon set for Wi-Fi, weather, BTC, ETH, and HYPE
- Region/dirty-state tracking where useful

Hardware checkpoints:

- [x] Phase 2A-1: Extract visible-coordinate mapping, pixel/fill-rectangle, and 5x7 text rendering into `GraphicsBW` without changing the Phase 1F refresh sequence (verified on hardware, 2026-09-29)
- [x] Phase 2A-2: Verify `drawLine()` and `drawRect()`, including geometry that crosses the visible x=396 dual-controller seam (verified on hardware, 2026-09-29)

**Phase 2A status: Complete**

See [docs/PHASE2_GRAPHICS.md](docs/PHASE2_GRAPHICS.md) for Phase 2 graphics verification notes.

## Phase 3 — Dashboard framework

**Goal:** Separate data from presentation and refresh only what needs to change.

Planned work:

- Dashboard layout system
- Widget interface
- Change detection / dirty regions
- Coalescing multiple data changes into one E-paper refresh
- Stable positioning for clock, weather, market prices, and status icons

## Phase 4 — Time and Wi-Fi status

**Goal:** Add the first live system data without external content APIs.

Planned work:

- Wi-Fi connection manager
- NTP time synchronization
- Local time handling
- Minute-level clock updates
- Wi-Fi RSSI measurement
- Upper-right Wi-Fi signal icon
- Reconnection behavior

## Phase 5 — Secure market data

**Goal:** Display BTC, ETH, and HYPE perpetual-futures prices.

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
