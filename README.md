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
             +-- DashboardState      (implemented)
             +-- Dashboard           (implemented)
             +-- ClockWidget         (planned)
             +-- WeatherWidget       (planned)
             +-- CryptoWidget        (planned)
             +-- WiFiWidget          (planned)
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

Phase 3A has extracted dashboard state and composition from the sketch into dedicated `DashboardState` and `Dashboard` objects. The Phase 2D layout and verified refresh behavior were preserved and re-verified on hardware on 2026-09-30.

The UI source files currently remain in the Arduino sketch directory so the existing Arduino build flow continues to discover and compile them without introducing a build-system change. Their architectural ownership is the UI layer.

Dirty-region ownership, widget change detection, and refresh coalescing remain Phase 3 responsibilities rather than GraphicsBW responsibilities.

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
│     ├─ DashboardState.h
│     ├─ Dashboard.h
│     ├─ Dashboard.cpp
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
