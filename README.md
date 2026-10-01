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
   - A compact signal-strength icon in the upper-right corner
   - Derived from the ESP32 Wi-Fi RSSI value

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
        |    +-- WiFiManager         (planned)
        |    +-- SecureHttpClient    (planned)
        |
        +-- services/
        |    +-- TimeService         (planned)
        |    +-- WeatherService      (planned)
        |    +-- MarketDataService   (planned)
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

Phase 3D-1 adds `DashboardUpdateCoalescer`. It owns durable displayed and pending snapshots, accepts either a complete dashboard state or widget-specific Clock/Weather/Wi-Fi/BTC/ETH/HYPE/Status updates, and recomputes the pending dirty mask against the last physically displayed state. Multiple widget updates can therefore accumulate in one pending state and be committed only after one successful physical refresh. If staged changes revert back to the displayed value before flush, their dirty bits disappear and the refresh can still be skipped. Hardware verification is pending.

Physical dirty-region refresh remains deferred. Phase 3D continues to use full-frame composition and the verified full-screen partial/maintenance refresh paths.

## Current repository structure

```text
EDP/
├─ README.md
├─ ROADMAP.md
├─ .gitignore
├─ docs/
│  ├─ HARDWARE.md
│  ├─ PHASE2_GRAPHICS.md
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
│     ├─ WiFiWidget.h
│     ├─ WiFiWidget.cpp
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
