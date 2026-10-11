# Phase 8B-5A — Dynamic Wi-Fi RSSI in the approved Dashboard

**Date:** 2026-10-11  
**Development branch:** `phase8a1-static-prototype`  
**Status:** Firmware changes committed and GitHub source/readback checks PASS. **Full Arduino IDE compile/upload and physical CrowPanel approval pending.** Phase 8B-5B production integration has **not started**.

## Why 8B-5 has two hardware gates

Phase 8B-4 already passed on the real CrowPanel for the new static-approved UI with **live NTP calendar, ETH/BTC/HYPE perpetuals and current weather**. However, that runtime is intentionally isolated and still displays an illustrative six-day weather strip and a fake STRONG Wi-Fi icon. Phase 8B-5A replaces **only the Wi-Fi bitmap** with a live indicator; Phase 8B-5B will later migrate the resulting compositor into `Dashboard::render()` / `PageRenderer` and preserve Phase 7 navigation/deep sleep / `DISPLAY CLEAN`. This staging avoids simultaneous driver/state/navigation changes.

## Wi-Fi model and pixel contract

No reimplementation of the Wi-Fi driver or RSSI thresholds. Reuse `WiFiManager::tick()`, `isConnected()`, `state()`, `rssi()` and the already verified Phase 4 `wifiIconForRssi()` function in `CrowPanelDashboard.ino`:

| State / RSSI (dBm) | Bitmap |
| --- | --- |
| Connected, RSSI ≥ -60 | `Icons::WIFI_STRONG` |
| Connected, -70 to -61 | `Icons::WIFI_MEDIUM` |
| Connected, -80 to -71 | `Icons::WIFI_WEAK` |
| Connected, RSSI ≤ -81 | `Icons::WIFI_VERY_WEAK` |
| DISCONNECTED or CONNECTING | `Icons::WIFI_DISCONNECTED` |

All assets are **existing** native 32×24 `Bitmap1bpp`. Their accepted position is **x=738, y=25**, with no font changes or service dependencies inside the renderer. A boot-only self-check validates the -60/-70/-80 boundary transitions and the five bitmap layouts.

The new graphics-only `Phase8A1Preview::renderWithCalendarMarketsWeatherWifi()` takes a referenced bitmap for the top right. The original `renderWithCalendarMarketsWeather()` continues using `Icons::WIFI_STRONG` as its **8B-4 regression fixture**, and 8A/8B-2/8B-3 wrappers remain unchanged. This is the same drawing code, not a second Dashboard implementation.

In the isolated test loop:

- Initial screen uses `WIFI_DISCONNECTED`, `WAITING`, `NO DATA`, market `--`, refreshed once using the verified startup `maintenanceRefresh()` behavior.
- After `WiFiManager::tick()`, poll the RSSI **once per 60 seconds**, and resample **immediately on connection-state transitions** (including DISCONNECTED / CONNECTING).
- Compare the selected immutable bitmap pointer, **not raw dBm**. When -58 changes to -59 (same strong icon), there is no Wi-Fi-only E-paper refresh; when -69 becomes -72 the icon changes.
- Hold the latest sampled bitmap pointer separately from the last **physically displayed** icon; if an E-paper partial refresh fails, the changed bitmap remains pending for retry.
- NTP local time, three USDⓈ-M perpetual public prices (60 sec) and actual Open-Meteo current weather (30 min) continue from Phase 8B-4 and are composed together with Wi-Fi into one framebuffer. Refresh owner stays in the application: `begin → restoreFrameStateForPartial(previousFrameBuffer) → displayPartialFrame(frameBuffer) → copy previousFrameBuffer → sleep`. No periodic `displayFullFrame()` and no E-paper service called from `WiFiManager`.

## Enable the 8B-5A experiment

Update `phase8a1-static-prototype`; in **ignored local** `config.h`, preserve the working Wi-Fi credentials and `WEATHER_LATITUDE` / `WEATHER_LONGITUDE`, then enable only:

```cpp
// #define EDP_PHASE8A1_PREVIEW 1
// #define EDP_PHASE8_FONT_TEST 1
// #define EDP_PHASE8_WEATHER_ICON_TEST 1
// #define EDP_PHASE8B2_CLOCK_TEST 1
// #define EDP_PHASE8B3_MARKET_TEST 1
// #define EDP_PHASE8B4_WEATHER_TEST 1

#define EDP_PHASE8B5A_WIFI_TEST 1
```

Compile and upload `CrowPanelDashboard.ino` in Arduino IDE, then open Serial Monitor at **115200 baud**. This test mode remains **isolated: buttons, page navigation, the DETAIL menu and deep sleep are not active** until 8B-5B, and the bottom six-day weather predictions remain **illustrative mocks**. Do not deploy the opt-in preview as a final production forecast.

Example serial output (RSSI and live data will vary):

```text
EDP Phase 8B-5A: LIVE NTP + CRYPTO + WEATHER + WIFI
PASS: 8B-5A weather, market, NTP and Wi-Fi RSSI self-checks.
PASS: 8B-5A WAITING / -- / NO DATA / Wi-Fi disconnected baseline.
8B-5A Wi-Fi icon sampled: DISCONNECTED / CONNECTING
8B-5A Wi-Fi icon sampled: -65 dBm -> MEDIUM
PASS: 8B-5A coalesced partial refresh (clock=changed, prices=changed, weather=changed, wifi=changed).
```

## Required hardware gate and scope

- [x] Source-level inspection and GitHub readback of new bitmap parameter, test-mode guard, connection transitions, 60-second RSSI samples and display-success-only Wi-Fi commit.
- [x] Previous approved fixed Wi-Fi art/assets, market/forecast coordinates, font assets and legacy entry points kept intact at source level.
- [ ] Arduino ESP32-S3 full sketch compilation and upload.
- [ ] Observe actual upper-right disconnected/connected bitmap and correct RSSI grade, with no clipping or ghosting.
- [ ] Verify NTP calendar, three crypto prices and current weather still operate; ensure no extra repaint for same RSSI grade.
- [ ] Optionally disconnect/reconnect Wi-Fi and check X icon and signal icon return; if not done, defer this to Phase 8B-6.
- [ ] Explicit real CrowPanel user acceptance before 8B-5B production migration.

**8B-5B remains pending.** It must migrate calendar and presentation dirty ownership into the application, verify startup, normal coalescer and page transitions, and exercise Phase 7 DETAIL DISPLAY CLEAN, standby and deep sleep. It must also stop illustrating six-day mock values as real production forecasts. The current regular `main` firmware and Phase 7 production code paths remain unchanged, as do secure HTTPS trust anchors, source services and SSD1683 maintenance logic.
