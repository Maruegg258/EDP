# Phase 8B-4 — Live Open-Meteo current weather

**Date:** 2026-10-11  
**Branch:** `phase8a1-static-prototype`  
**Status:** Isolated source implementation committed; independent GitHub readback/source checks completed. **Arduino IDE ESP32-S3 compile and physical CrowPanel validation are pending.**

## Implementation boundary

The user accepted the Phase 8A visuals, 8B-2 NTP calendar and 8B-3 live BTC/ETH/HYPE price row. Phase 8B-4 changes only the **upper-left current-weather block** of that exact 792×272 composition, plus the test-mode application input flow. The 34/17/14px JetBrains Mono glyph assets, 11×32px user-approved weather icons, six-day mock strip, time/market positions, graphic Wi-Fi mock indicator and divider lines remain unchanged.

Actual current-weather fields come from the **already verified** `WeatherService::lastValidSnapshot()->current`:

- `temperatureC` → native 14px, one decimal plus ` C` (e.g. `22.7 C`).
- `condition` → concise, ≤10-character English uppercase 17px label and one approved 32×32 bitmap.
- `isDay` → selects day/night artwork where relevant, even if the numeric temperature and text are unchanged.
- `time`/`weatherCode` remain source metadata, not a separate E-paper rendering concern.

No new weather API endpoint, weather provider contract, NTP client, background service, HTTPS policy, BLE/OTA/MQTT or UI refresh owner is introduced. Reuse Open-Meteo `WeatherService` with ISRG Root X1 validated TLS and its existing **30-minute** poll interval. Prices continue using the established Binance USDⓈ-M read-only endpoints and DigiCert G2 trust root at **60-second** intervals; NTP remains local `CST-8`.

## Bounded presentation and labels

`Phase8WeatherPresentation.h/.cpp` owns `Phase8CurrentWeather`: `const Bitmap1bpp*` referencing a static approved glyph, `char label[11]`, `char temperature[12]`. `fromCurrent()` rejects invalid/nonfinite/unrepresentable temperatures without mutating prior output; `setUnavailable()` creates an explicit `NO DATA`, `-- C`, UNKNOWN-icon placeholder. The `equal()` test compares **icon pointer plus actual label and temperature text**. The drawing code accepts only 32×32 bitmaps, a max ten-glyph label (≤120px at 17px), and text fitting the approved header anchors.

| Weather category | Header label | Icon selection |
| --- | --- | --- |
| CLEAR | SUNNY (day) / CLEAR (night) | Sun / crescent |
| MAINLY_CLEAR | MAINLY CLR | Sun / crescent |
| PARTLY_CLOUDY | PARTLY CLD | Partial sun/cloud or moon/cloud |
| OVERCAST / FOG | OVERCAST / FOG | Cloud / fog |
| DRIZZLE / FREEZING_DRIZZLE | DRIZZLE / FRZ DRZL | Drizzle |
| RAIN / FREEZING_RAIN / RAIN_SHOWERS | RAIN / FRZ RAIN / SHOWERS | Rain |
| SNOW / SNOW_GRAINS / SNOW_SHOWERS | SNOW / SNOW GRAIN / SNOW SHWRS | Snow |
| THUNDERSTORM / THUNDERSTORM_HAIL | T-STORM / T-STORM HL | Thunder |
| UNKNOWN | UNKNOWN | Question mark |

New `Phase8WeatherPresentation::selfCheck()` exercises all 16 categories with day and night, valid decimal and negative temperatures, invalid numeric input preserving prior output, and the explicit no-data state. The previously accepted `WeatherWidgetMapper.cpp` remains entirely untouched for the older production Dashboard.

## Shared graphics-only renderer

`Phase8A1Preview::renderWithCalendarMarketsWeather(GraphicsBW&, const Phase8CalendarHeader&, const Phase8MarketPrices&, const Phase8CurrentWeather&)` is the new four-slice data entry point. It uses the **same drawing composition**, offsets and divider rules as Phase 8A:

- Upper-left approved weather icon x=22,y=25; label x=69,y=27; temperature x=69,y=48.
- Central local date / time / weekday group unchanged.
- Middle ETH/BTC/HYPE 34px prices and three 248px column positions unchanged.
- Bottom six mock forecast columns unchanged.

Previously accepted entry points remain: `render()` full-static Phase 8A, `renderWithCalendar()` Phase 8B-2 live calendar with mock markets, `renderWithCalendarAndMarkets()` Phase 8B-3 live calendar+markets with **fixed SUNNY/26.9 C**. These entry points delegate to the same parameterized compositor rather than implementing another Dashboard.

## How to test on CrowPanel

Check out/update `phase8a1-static-prototype`, open its original `CrowPanelDashboard.ino` in Arduino IDE, and in **ignored local `config.h`** preserve real credentials and weather coordinates. **All other test flags must be disabled**:

```cpp
// #define EDP_PHASE8A1_PREVIEW 1
// #define EDP_PHASE8_FONT_TEST 1
// #define EDP_PHASE8_WEATHER_ICON_TEST 1
// #define EDP_PHASE8B2_CLOCK_TEST 1
// #define EDP_PHASE8B3_MARKET_TEST 1
#define EDP_PHASE8B4_WEATHER_TEST 1

// Also keep existing actual values (in config.h only):
// #define WEATHER_LATITUDE  <your latitude>
// #define WEATHER_LONGITUDE <your longitude>
```

The new flag requires both `WEATHER_LATITUDE` and `WEATHER_LONGITUDE` macros; Arduino compilation deliberately emits an informative `#error` if omitted. Never commit local `config.h` or Wi-Fi credentials.

After compile/upload, open Serial Monitor at **115200**:

1. The application self-checks calendar, market formatter, icon semantics and the 16-category weather presentation; expect `PASS: 8B-4 all presentation/icon self-checks.`
2. It renders initial `WAITING`, market `--` and weather `NO DATA / -- C` with the existing single **maintenanceRefresh()** baseline.
3. After Wi-Fi + NTP, it fetches three Binance perpetual prices, then **existing** Open-Meteo weather. In the upper left, the UNKNOWN placeholder becomes a real temperature, concise condition label, and the applicable day/night icon. Bottom six days and top-right Wi-Fi glyph remain **STATIC MOCKS**.
4. Later cycles poll prices each 60s and weather each 30min (only if connected and NTP valid). The candidate calendar, three prices and current weather are compared against the **last physically displayed** values. Changes are composed into one framebuffer then application-owned `begin → restoreFrameStateForPartial → displayPartialFrame → copy previousFrameBuffer → sleep`. No added full refresh is scheduled.
5. If Open-Meteo fails, the previously valid `lastValidSnapshot` stays preserved. If none is available, retain `NO DATA`. No API/service triggers display updates directly; a failed refresh does **not** mark the candidate displayed.

Sample Serial messages (actual prices/temperatures vary):

```text
EDP Phase 8B-4: LIVE NTP + CRYPTO + CURRENT WEATHER
PASS: 8B-4 all presentation/icon self-checks.
PASS: 8B-4 WAITING / -- / NO DATA shown.
8B-3: polling three existing USD-M perpetual symbols.
8B-4: polling existing Open-Meteo current weather (30 min).
Weather fetched: CLEAR DAY SUNNY 26.9 C
PASS: 8B-4 coalesced partial refresh (clock=changed, prices=changed, weather=changed).
```

### Physical acceptance focus

- [ ] Arduino IDE compile, upload, serial self-check PASS and actual Open-Meteo weather fetch.
- [ ] Compare current weather icon/temperature/description with the Serial output; confirm day/night icon (sun versus crescent when CLEAR), crisp 17px/14px and no collision with the central date.
- [ ] Verify NTP time, date, ETH/BTC/HYPE prices still update normally.
- [ ] Wait for at least one minute roll; no blurred text or ghosting near the dual-controller seam.
- [ ] If feasible, inspect a second weather poll after ~30 minutes; unchanged visual weather should not cause a redraw by itself.
- [ ] Optional failure and dawn/dusk transition checks can be deferred to Phase 8B-6; **do not claim these are already hardware verified**.

**Scope not covered:** Real six-day daily forecast (`WeatherSnapshot::DAILY_RESPONSE_COUNT=2` currently), live RSSI icon, production Dashboard `DashboardState` migration, PageRenderer, DETAIL page actions, deep sleep or overnight ghosting. These are 8C, 8B-5 and 8B-6. `main` and pre-Phase-8 application behavior remain unchanged.
