# Phase 4 system-data verification

This document records hardware-verified Phase 4 behavior for Wi-Fi and time-related system data.

## Phase 4A-1 — Wi-Fi connection manager

**Status: Verified on hardware (2026-10-03)**

Implementation:

- Project-owned `WiFiManager`
- ESP32 station mode only
- Credentials loaded from local `config.h`
- `config.h` remains excluded from Git
- Non-blocking connection state machine
- Connection states: `DISCONNECTED`, `CONNECTING`, `CONNECTED`
- Connection timeout followed by delayed retry
- RSSI available only while connected
- No OTA, web server, MQTT, BLE, ESP-NOW, telemetry, or other background network service added

Hardware verification confirmed:

- Firmware compiles and uploads successfully after the Phase 4A-1 declaration fix
- Device connects using local Wi-Fi credentials
- Serial reports connection state changes
- Serial reports live RSSI values
- Disconnect/reconnect behavior works without a blocking wait loop
- E-paper refresh is intentionally not invoked by the Phase 4A-1 test application
- Existing display driver and verified refresh sequence were not modified

Phase 4A-1 does not yet feed live Wi-Fi data into the dashboard UI. That integration is Phase 4A-2.

## Phase 4A-2 — Live Wi-Fi dashboard integration

**Status: Verified on hardware (2026-10-03)**

Implementation:

- Upper-right Wi-Fi widget renders only a compact bar-style icon
- Removed the visible `WIFI` label and numeric RSSI text
- RSSI remains internal/Serial diagnostic data
- Four connected signal bands:
  - `>= -60 dBm`: four solid bars
  - `-61..-70 dBm`: three solid bars
  - `-71..-80 dBm`: two solid bars
  - `< -80 dBm`: one solid bar
- Inactive bars are hollow to preserve the reference style on a 1-bit display
- Disconnected state uses hollow bars plus an X
- Connection-state changes are handled immediately
- Connected RSSI is sampled for UI state once per minute
- `WiFiWidgetState` stores only the visible icon, so RSSI movement inside the same band does not mark the dashboard dirty
- Updates enter the existing `DashboardUpdateCoalescer::stageWiFi()` path
- If the resulting icon is unchanged, E-paper activity is skipped
- Display-driver and refresh-sequence code are unchanged

Hardware verification confirmed the icon-only Wi-Fi presentation and live integration behave correctly on the panel. The bar-style icon is clear and readable in the current upper-right position. Final position/spacing polish is deferred to Phase 8 UI refinement.
