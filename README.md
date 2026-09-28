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
        |    +-- GraphicsBW
        |    +-- Fonts               (planned)
        |    +-- Icons               (planned)
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
             +-- Dashboard           (planned)
             +-- ClockWidget         (planned)
             +-- WeatherWidget       (planned)
             +-- CryptoWidget        (planned)
             +-- WiFiWidget          (planned)
```

Display drivers must not know about Wi-Fi, HTTP, market data, or weather services. Data services must not directly control E-paper refreshes. UI code decides what changed and the display layer performs the required refresh.

## Current repository structure

```text
EDP/
├─ README.md
├─ ROADMAP.md
├─ .gitignore
├─ docs/
│  ├─ HARDWARE.md
│  ├─ REFRESH_NOTES.md
│  └─ SECURITY.md
├─ firmware/
│  └─ CrowPanelDashboard/
│     ├─ CrowPanelDashboard.ino
│     ├─ config.example.h
│     ├─ EpaperBus.h
│     ├─ EpaperBus.cpp
│     ├─ CrowEPD579.h
│     ├─ CrowEPD579.cpp
│     ├─ GraphicsBW.h
│     └─ GraphicsBW.cpp
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
