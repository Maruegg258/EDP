# EDP

Custom firmware for the Elecrow CrowPanel ESP32 5.79-inch E-paper HMI display.

The long-term goal is a modular, low-power **E-paper dashboard platform**, not a crypto-only display. The repository is the source of truth for firmware, hardware notes, security rules, refresh behavior, and project progress.

## Product goals

The finished dashboard is planned to support:

1. **Weather information**
   - Current conditions and temperature
   - Weather icons
   - Later expansion to items such as feels-like temperature or precipitation probability

2. **Time**
   - Network-synchronized time
   - Minute-level display updates suitable for E-paper
   - No second-by-second refresh by default

3. **Perpetual futures prices**
   - BTC
   - ETH
   - HYPE
   - Market data should live in a dedicated service layer rather than in the display driver

4. **Wi-Fi signal indicator**
   - A compact bar-style signal-strength icon in the upper-right corner
   - Derived from the ESP32 Wi-Fi RSSI value; numeric RSSI is not displayed

5. **Improved typography and small icons**
   - Custom bitmap fonts where useful
   - Weather, Wi-Fi, and crypto icons
   - Visual polish while preserving E-paper readability and refresh efficiency

6. **Multi-function physical button controls**
   - Planned support for actions such as short press, long press, and double press
   - Potential uses include page switching, manual refresh, status/settings views, and network actions

## Architecture principles

The firmware should keep hardware, data, and UI concerns separate:

```text
CrowPanelDashboard.ino
        |
        +-- drivers/
        |    +-- EpaperBus
        |    +-- CrowEPD579
        |    +-- ButtonManager       (planned)
        |
        +-- graphics/
        |    +-- GraphicsBW          (implemented)
        |    +-- BitmapFont          (implemented)
        |    +-- Font5x7 / Font9x13  (implemented)
        |    +-- Bitmap1bpp          (implemented)
        |    +-- Icons               (implemented)
        |
        +-- network/
        |    +-- WiFiManager         (implemented; Phase 4A-1 hardware verified)
        |    +-- SecureHttpClient    (implemented; Phase 5A-1 hardware verified)
        |    +-- TlsTrustAnchors     (implemented; DigiCert Global Root G2 baseline)
        |
        +-- services/
        |    +-- TimeService         (implemented; Phase 4 hardware verified)
        |    +-- WeatherService      (Phase 6A-2 hardware verified)
        |    +-- WeatherCondition    (Phase 6A-3 hardware verified)
        |    +-- MarketDataService   (BTC/ETH/HYPE polling hardware verified)
        |
        +-- ui/
             +-- WidgetStates        (implemented)
             +-- DashboardDirty      (implemented)
             +-- WidgetStateCompare  (implemented)
             +-- DashboardStateCompare (implemented)
             +-- DashboardStateSnapshot (implemented)
             +-- DashboardUpdateCoalescer (implemented)
             +-- DashboardState      (implemented)
             +-- Dashboard           (implemented)
             +-- ClockWidget         (implemented)
             +-- WeatherWidget       (implemented)
             +-- WeatherWidgetMapper (Phase 6B-1 hardware verified)\n             +-- Weather polling      (Phase 6B-2 hardware verified; 30-minute cadence)
             +-- CryptoWidget        (implemented)
             +-- WiFiWidget          (implemented)
             +-- StatusWidget        (implemented)
```

Display drivers must not know about Wi-Fi, HTTP, market data, or weather services. Data services must not directly control E-paper refreshes. UI code decides what changed and the display layer performs the required refresh.


## Phase 2 graphics baseline

Phase 2 is hardware-verified and complete as of 2026-09-30.

The graphics layer now owns:

- visible 792x272 coordinate rendering over the raw 800x272 dual-controller framebuffer
- pixel, line, outline rectangle, and filled rectangle primitives
- generic bitmap-font rendering through `BitmapFont`
- project-owned `Font5x7` and `Font9x13`
- generic transparent 1-bit bitmap rendering through `Bitmap1bpp`
- project-owned Wi-Fi, weather, BTC, ETH, and HYPE icon assets

The display driver remains responsible only for controller/RAM/refresh behavior. The known-good maintenance sequence remains:

`Fast clear -> physical white -> re-init -> previous RAM white -> current RAM new frame -> Partial refresh`

Phase 3A-1 extracted dashboard state and composition from the sketch into dedicated `DashboardState` and `Dashboard` objects. The Phase 2D layout and verified refresh behavior were preserved and re-verified on hardware on 2026-09-30.

Phase 3A-2 extracted the static dashboard regression content into `DashboardTestStates.h`, leaving the application sketch responsible for sequencing rendering and display refresh rather than owning UI test values. The unchanged full -> partial -> maintenance -> three partial regression sequence was verified on hardware on 2026-09-30.

The UI source files currently remain in the Arduino sketch directory so the existing Arduino build flow continues to discover and compile them without introducing a build-system change. Their architectural ownership is the UI layer.

Phase 3B-1 extracted the clock, weather, Wi-Fi, and crypto rendering blocks into dedicated widgets. BTC, ETH, and HYPE share one reusable `CryptoWidget` implementation. The unchanged full -> partial -> maintenance -> three partial regression sequence was verified on hardware on 2026-10-01.

Phase 3B-2 introduces widget-specific state objects through `WidgetStates.h` and moves the dynamic bottom status bar into a dedicated `StatusWidget`. `DashboardState` now composes widget states, while `Dashboard` only owns static frame chrome and widget composition. This boundary was verified on hardware on 2026-10-01 with the unchanged full -> partial -> maintenance -> three partial regression sequence and is the basis for Phase 3C widget-local change detection.

Phase 3C-1 adds content-based widget-state comparison and a logical dashboard dirty bitmask for clock, weather, Wi-Fi, BTC, ETH, HYPE, and status. Text values are compared by content rather than pointer address; project-owned weather/Wi-Fi bitmap assets are compared by object identity. The Serial dirty self-check and the unchanged full -> partial -> maintenance -> three partial display regression were verified on hardware on 2026-10-01. This phase does not yet skip refreshes or perform physical dirty-region updates.

Phase 3C-2 adds `DashboardStateSnapshot`, which owns fixed-size copies of every dynamic text field while retaining project-owned weather/Wi-Fi bitmap references. Snapshot capture validates capacity before copying, so a failed capture does not partially replace the previous logical state. A Serial self-check mutates the original live buffers after capture to verify that dirty detection still compares against the preserved previous values. The snapshot self-check and unchanged six-stage display regression were verified on hardware on 2026-10-01.

Phase 3C-3 applies the logical dirty result to the application flow. When the incoming state matches the durable previous-state snapshot, the firmware skips framebuffer rendering, display wake/reset, RAM restore, and physical refresh. Changed states still compose the full framebuffer and use the already verified full-screen partial or maintenance refresh path. The regression intentionally repeats unchanged states around baseline, partial, maintenance, and final frames so the skip behavior can be observed on hardware. The skip behavior and changed-state refresh path were verified on hardware on 2026-10-01.

Phase 3C is therefore complete: the UI can identify which widgets changed, preserve a durable previous logical state, and avoid unnecessary E-paper activity when the incoming dashboard state is unchanged.

Phase 3D-1 adds `DashboardUpdateCoalescer`. It owns durable displayed and pending snapshots, accepts either a complete dashboard state or widget-specific Clock/Weather/Wi-Fi/BTC/ETH/HYPE/Status updates, and recomputes the pending dirty mask against the last physically displayed state. Multiple widget updates can therefore accumulate in one pending state and be committed only after one successful physical refresh. If staged changes revert back to the displayed value before flush, their dirty bits disappear and the refresh can still be skipped. The coalescer self-check, seven-widget single-refresh behavior, unchanged-state skips, and existing refresh quality were verified on hardware on 2026-10-01.

Phase 3D-2 adds an integration regression around the existing coalescer rather than expanding its API. The checks verify that staged pending data owns copied text even if source buffers mutate, discarding pending work leaves the displayed snapshot unchanged, committing advances the displayed snapshot only after a successful flush, and dirty bits can accumulate and then cancel back to NONE before any display activity. The ownership/discard checks, dirty-cancel skip behavior, coalesced refresh behavior, and existing full-frame partial/maintenance paths were verified on hardware on 2026-10-01.

Phase 3 is complete as of 2026-10-01. The dashboard framework now provides modular widgets, logical dirty tracking, durable displayed/pending state ownership, skip-unchanged behavior, and refresh coalescing. Physical dirty-region refresh remains deliberately deferred; the project continues to use full-frame composition and the verified full-screen partial/maintenance refresh paths.

Phase 4 introduces the first live system data: Wi-Fi connection/RSSI and NTP-synchronized local time, routed through the existing Phase 3 state/coalescing architecture rather than directly controlling the display.\n\nPhase 4A-1 adds a project-owned non-blocking `WiFiManager` that uses local `config.h` credentials, reports connection state and RSSI through Serial, and retries after failed/lost connections without a blocking wait loop. The Phase 4A-1 hardware-test entry point intentionally performs no E-paper activity; the existing display driver and verified refresh sequences remain unchanged. Compile/upload, live connection, RSSI reporting, and reconnect behavior were verified on hardware on 2026-10-03.\n\nPhase 4A-2 maps live RSSI into four visible bar levels plus a disconnected state. `WiFiWidget` now renders only the icon: the `WIFI` label and numeric RSSI were removed. RSSI is sampled for the UI once per minute and connection-state transitions are handled immediately; the Phase 3 coalescer skips physical refresh when the resulting icon is unchanged. Icon-only rendering and live Wi-Fi integration were verified on hardware on 2026-10-03. Final icon positioning is intentionally deferred to Phase 8 UI refinement.

Phase 4B-1 adds a project-owned `TimeService` that starts SNTP through explicitly configured NTP servers, applies an explicit Taiwan POSIX timezone rule (`CST-8`, UTC+8 with no DST), and exposes synchronized local time without depending on Dashboard or E-paper code. The hardware-test application reports time through Serial only and intentionally performs no display activity. NTP synchronization, Taiwan local-time output, continued system-clock progression during Wi-Fi loss, and Wi-Fi recovery were verified on hardware on 2026-10-03.

Phase 4B-2 connects synchronized minute-level local time to the existing Clock widget. At each new minute the application stages both Clock and a fresh Wi-Fi visual sample before one coalesced flush. If the Wi-Fi icon stays in the same signal band, only `CLOCK` is dirty; if the signal band changes in the same cycle, the pending mask becomes `CLOCK | WIFI` and still produces one physical partial refresh. Wi-Fi connection-state transitions remain immediate. Live `HH:MM`, minute-level refresh behavior, Clock/Wi-Fi coalescing, Wi-Fi outage/recovery behavior, and refresh quality were verified on hardware on 2026-10-03.

Phase 4 is complete as of 2026-10-03. The firmware now has hardware-verified non-blocking Wi-Fi management, icon-only signal status, NTP-synchronized Taiwan local time, continued clock progression during temporary network loss, and application-level Clock/Wi-Fi refresh coalescing.

Phase 5A-1 introduces a project-owned `SecureHttpClient` and a public `DigiCert Global Root G2` trust anchor. The application waits for both Wi-Fi and synchronized system time, then performs one certificate-validating HTTPS GET to the Binance USDⓈ-M Futures BTC ticker endpoint and reports the result through Serial only. No market value is staged into the dashboard in this checkpoint, and no `setInsecure()` fallback exists. Compile/upload, NTP-before-HTTPS gating, certificate-validating HTTPS, HTTP 200/BTC ticker response, and coexistence with the Phase 4 Clock/Wi-Fi behavior were verified on hardware on 2026-10-04.

Phase 5A-2 adds a dedicated `MarketDataService` above `SecureHttpClient`. The initial implementation remains BTC-only and Serial-only: it fetches `BTCUSDT`, strictly validates the returned symbol and positive finite price, accepts the optional numeric source-time field, and commits a new last-valid snapshot only after all validation succeeds. The test flow then intentionally requests an invalid symbol to verify that a later HTTP failure does not clear or mutate the valid BTC snapshot. Compile/upload, BTC parsing, source-time parsing, expected invalid-symbol failure, last-valid preservation, and coexistence with the existing Clock/Wi-Fi dashboard behavior were verified on hardware on 2026-10-04. Crypto widgets remained unchanged as intended.

Phase 5A-3 expands the service from one shared last-valid value to three independent configured slots for `BTCUSDT`, `ETHUSDT`, and `HYPEUSDT`. The application performs a 60-second Serial-only polling cycle after the Clock/Wi-Fi display flush, so market HTTPS work does not take priority over minute-level UI updates. Each symbol is fetched and validated independently; a failed symbol request leaves that symbol's previous valid snapshot intact while the other symbols continue to update. Compile/upload, repeated 60-second polling, valid BTC/ETH/HYPE parsing, three independent stored market values, and coexistence with the existing Clock/Wi-Fi display behavior were verified on hardware on 2026-10-04. No market value was staged into the dashboard in this checkpoint.

Phase 5B-1 connects validated market values to the existing BTC/ETH/HYPE `CryptoWidgetState` objects through `DashboardUpdateCoalescer::stageBtc() / stageEth() / stageHype()`. Startup crypto fields now show `--` until the first valid market value arrives. A successful market fetch stages only the returned price string; the coalescer's existing content comparison decides whether that price creates a BTC/ETH/HYPE dirty bit. Failed market fetches stage nothing, so an already displayed last-valid price remains visible. The market service still never calls the display driver or triggers a refresh directly. The existing application flush consumes staged market changes through the verified refresh path. Compile/upload, startup placeholders, live BTC/ETH/HYPE display values, coalesced Crypto dirty staging, continued 60-second market polling, normal Clock/Wi-Fi behavior, and clear refresh quality were verified on hardware on 2026-10-04. Explicit unchanged-price and induced failure/recovery tests remain for Phase 5B-2.

Phase 5B-2 adds a one-shot application-level diagnostic state machine without changing `MarketDataService`, `SecureHttpClient`, or the display driver. After the first live prices are physically displayed, it re-stages the exact same BTC/ETH/HYPE values and requires `DashboardDirty::NONE` with no pending refresh. It then simulates ETH market data being unavailable for one polling cycle by skipping only the ETH fetch/stage step; BTC and HYPE continue normally, while both the displayed ETH value and the service's existing ETH last-valid snapshot must remain unchanged. A normal ETH request is then forced immediately for recovery verification. Compile/upload, live unchanged-price no-dirty behavior, ETH last-valid hold, independent BTC/HYPE continuation, ETH recovery, normal Clock/Wi-Fi behavior, and E-paper clarity were verified on hardware on 2026-10-04. Because short-lived failures are handled safely by last-valid retention, no stale-data marker is required for Phase 5; prolonged-outage indication is deferred to Phase 9 reliability work. The temporary diagnostic injection still needs to be removed before Phase 5 is closed.

Phase 5B-3 removes the temporary Phase 5B-2 boot-time diagnostic state machine and its simulated ETH-unavailable cycle from the application. The production path now contains only the normal 60-second BTC/ETH/HYPE fetch loop, per-symbol validation/last-valid preservation, Crypto-widget staging through `DashboardUpdateCoalescer`, and the existing application-controlled E-paper flush. `MarketDataService`, TLS handling, coalescer internals, and display-driver refresh behavior are unchanged. The cleaned production path was verified on hardware on 2026-10-04: no diagnostic injection remained, live BTC/ETH/HYPE pricing and 60-second polling were normal, Clock/Wi-Fi behavior remained normal, and E-paper refresh quality remained clear.

Phase 5 is complete as of 2026-10-04. The firmware now has hardware-verified certificate-validating Binance USDⓈ-M market retrieval, independent last-valid BTC/ETH/HYPE state, live Crypto widget integration through the coalescer, unchanged-value refresh suppression, short-failure hold/recovery behavior, and a production path free of diagnostic injection. Prolonged market-data outage/staleness indication is intentionally deferred to Phase 9 reliability hardening.

## Current repository structure

```text
EDP/
├─ README.md
├─ ROADMAP.md
├─ .gitignore
├─ docs/
│  ├─ HARDWARE.md
│  ├─ PHASE2_GRAPHICS.md
│  ├─ PHASE4_SYSTEM.md
│  ├─ REFRESH_NOTES.md
│  └─ SECURITY.md
├─ firmware/
│  └─ CrowPanelDashboard/
│     ├─ CrowPanelDashboard.ino
│     ├─ WidgetStates.h
│     ├─ DashboardDirty.h
│     ├─ WidgetStateCompare.h
│     ├─ WidgetStateCompare.cpp
│     ├─ DashboardStateCompare.h
│     ├─ DashboardStateCompare.cpp
│     ├─ DashboardStateSnapshot.h
│     ├─ DashboardStateSnapshot.cpp
│     ├─ DashboardUpdateCoalescer.h
│     ├─ DashboardUpdateCoalescer.cpp
│     ├─ DashboardState.h
│     ├─ DashboardTestStates.h
│     ├─ Dashboard.h
│     ├─ Dashboard.cpp
│     ├─ UiText.h
│     ├─ ClockWidget.h
│     ├─ ClockWidget.cpp
│     ├─ WeatherWidget.h
│     ├─ WeatherWidget.cpp
│     ├─ WeatherWidgetMapper.h
│     ├─ WeatherWidgetMapper.cpp
│     ├─ WiFiWidget.h
│     ├─ WiFiWidget.cpp
│     ├─ WiFiManager.h
│     ├─ WiFiManager.cpp
│     ├─ TimeService.h
│     ├─ TimeService.cpp
│     ├─ WeatherService.h
│     ├─ WeatherService.cpp
│     ├─ WeatherCondition.h
│     ├─ WeatherCondition.cpp
│     ├─ SecureHttpClient.h
│     ├─ SecureHttpClient.cpp
│     ├─ TlsTrustAnchors.h
│     ├─ MarketDataService.h
│     ├─ MarketDataService.cpp
│     ├─ CryptoWidget.h
│     ├─ CryptoWidget.cpp
│     ├─ StatusWidget.h
│     ├─ StatusWidget.cpp
│     ├─ config.example.h
│     ├─ EpaperBus.h
│     ├─ EpaperBus.cpp
│     ├─ CrowEPD579.h
│     ├─ CrowEPD579.cpp
│     ├─ Bitmap1bpp.h
│     ├─ BitmapFont.h
│     ├─ Font5x7.h
│     ├─ Font9x13.h
│     ├─ GraphicsBW.h
│     ├─ GraphicsBW.cpp
│     ├─ Icons.h
│     └─ TestBitmaps.h
└─ references/
   └─ README.md
```

## Development rules

- Tested changes should be committed with descriptive messages.
- Local secrets such as Wi-Fi credentials must never be committed.
- Production HTTPS must validate TLS certificates.
- Refresh behavior that is confirmed on real hardware should be documented in `docs/REFRESH_NOTES.md`.
- Hardware/controller assumptions should be distinguished from verified facts.
- New features should be added incrementally so each working stage can be recovered from Git history.

See [ROADMAP.md](ROADMAP.md) for the planned implementation sequence.
