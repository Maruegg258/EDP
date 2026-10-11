# Phase 8B-2 — NTP calendar presentation and isolated on-panel test

**Date:** 2026-10-11  
**Branch:** `phase8a1-static-prototype`  
**Status:** Code committed; GitHub readback and static checks PASS; **Arduino IDE compile/upload and user hardware verification PENDING**.

## What is implemented

Phase 8A's user-approved `Phase8A1Preview.cpp` remains the **single artwork/layout implementation**. It now has two drawing entry points:

- `Phase8A1Preview::render(GraphicsBW&)`: unchanged visual appearance from Phase 8A — immutable `12:59`, `10 OCT`, `SUNDAY` test fixture and all fixed weather/crypto/UI pixels. Remains offline.
- `Phase8A1Preview::renderWithCalendar(GraphicsBW&, const Phase8CalendarHeader&)`: accepts an **owned, non-pointer** 6+7+10 byte header (`HH:MM`, `DD MMM`, full uppercase weekday). Only the date, time and weekday change; all other graphics are exactly the prior accepted Phase 8A mock values.

The new `Phase8CalendarPresentation.h/.cpp` formats a `struct tm` obtained from the **existing** `TimeService::getLocalTime()`, rather than copying raw provider or NTP clock responsibilities into UI. It uses explicit English month/weekday names, so locale settings do not alter glyph coverage. A source-level self-test exercises October 11, minute change, December 31 → January 1 rollover, the longest `WEDNESDAY` glyph string, and invalid calendar inputs. `fromLocalTm()` leaves the last accepted value unchanged when input validation fails.

The accepted JetBrains Mono 34px/17px/14px glyphs, icons, dividers, Crypto center alignment, six-day forecast, Wi-Fi drawing and current physical refresh driver are unchanged. The extra calendar state is presently owned by the isolated test mode, **not** yet part of `DashboardState`/`DashboardUpdateCoalescer`. That ownership must be addressed deliberately before enabling this as the production Dashboard.

## Isolated test mode (only live service = NTP)

In your **ignored local** `firmware/CrowPanelDashboard/config.h`, keep Wi-Fi credentials and enable exactly the Phase 8B-2 mode:

```cpp
// #define EDP_PHASE8A1_PREVIEW 1
// #define EDP_PHASE8_FONT_TEST 1
// #define EDP_PHASE8_WEATHER_ICON_TEST 1
#define EDP_PHASE8B2_CLOCK_TEST 1
```

Arduino IDE: compile/upload the normal `CrowPanelDashboard.ino` from `phase8a1-static-prototype`, open Serial Monitor at **115200 baud**. No Python generation command is necessary.

Expected behavior:

1. On startup, the application runs the calendar formatter self-check. It renders the accepted Dashboard **once** using `--:--`, `-- ---`, `WAITING` in its header to mark uninitialized NTP, using existing `maintenanceRefresh(frameBuffer)`. The other content remains static mock data.
2. After Wi-Fi association, it calls the current production `startTimeServiceWhenConnected()` using the existing `TAIPEI_TIMEZONE = "CST-8"` and `time.cloudflare.com` / `pool.ntp.org`. No MarketDataService, WeatherService or HTTP/TLS poll is triggered in the test mode.
3. Once `TimeService` reports synchronized, use its real local `tm` to build owned `HH:MM`, `DD MMM`, `WEEKDAY` strings. Compare **all three fields** to the previously physically displayed values. The one already-existing application refresh choreography is used: `display.begin()` → `restoreFrameStateForPartial(previousFrameBuffer)` → `displayPartialFrame(frameBuffer)` → previous frame copy → `display.sleep()`. The displayed calendar state changes **only after physical refresh succeeds**.
4. No repeated physical refresh while header content is unchanged. Subsequent minute changes, NTP clock corrections, and local day/week/month changes are detected from rendered strings, not from an untracked epoch counter. Wi-Fi reconnects are managed by the existing `WiFiManager`; an already synchronized local clock can continue advancing without repeated internet access.
5. On failure to receive NTP, the header stays `WAITING`, rather than claiming any fabricated time. On a display or rendering error, the last displayed state/frame remains committed and can be retried. The existing `Phase8A1Preview` static and weather/font Gallery diagnostic modes are still available by changing the flag.

Serial examples (the actual time/date vary):

```text
EDP Phase 8B-2: NTP TIME / DATE / WEEKDAY TEST
PASS: calendar formatter, midnight and weekday self-check.
PASS: 8B-2 WAITING baseline displayed via maintenanceRefresh().
Waiting for NTP; only TimeService and Wi-Fi reconnect are active.
PASS: 8B-2 calendar refreshed: 11 OCT SUNDAY 14:02
```

**Important:** Even though NTP is live, the **Crypto prices, weather condition/temperature, six daily forecast rows and displayed Wi-Fi strength are still static examples**. This is an intermediate isolated NTP experiment, **not** a finished live Dashboard.

## Acceptance checklist

- [x] New owned and bounded `Phase8CalendarHeader` and locale-independent converter.
- [x] Built-in calendar self-check exercises minute, year/date/weekday rollover and invalid inputs.
- [x] Reuse `Phase8A1Preview` layout for static and NTP-header composition; accepted static fixture preserved.
- [x] Test flag mutually exclusive with existing static/font/weather icon modes.
- [x] Uses existing Wi-Fi/NTP services without enabling Market/Weather API polling or new background network services.
- [x] Compares actual visible header strings and commits only after verified physical refresh.
- [x] GitHub source readback and static regression checks; standalone C++11 calendar code equivalent compiled with `g++ -Wall -Wextra -Werror -pedantic` and self-check passed (this is **not** an ESP32 Arduino build).
- [ ] ESP32-S3 Arduino IDE compile and upload.
- [ ] Real panel: current Taipei HH:MM, day/month and weekday render correctly.
- [ ] Real panel: one or more minute changes remain crisp, with no unnecessary extra refresh.
- [ ] Observe Wi-Fi disconnected / unsynchronized behavior; verify placeholders and reconnect.
- [ ] Confirm Phase 8A immutable fixture remains unchanged by switching flags back.
- [ ] Later: calendar dirty-state ownership in normal application, without introducing another display-update scheduler.

### Known scope boundaries

This is **Phase 8B-2's first device gate**, not the full migration of `Dashboard::render()` to Phase 8A layout. The normal Phase 7 `DashboardState`, dirty/coalescer, page routing, market/weather services, DETAIL menu, DISPLAY CLEAN and deep sleep are deliberately untouched. A future Phase 8B production integration must migrate **all** physical render entry points to the shared design and give date/weekday an application-owned dirty contract, preserving the established refresh sequence.

The six mock forecast cells remain fictional until Phase 8C acquires real daily data. Do not mistake them for live predictions.
