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
