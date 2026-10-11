# Phase 8C-3 — Full live Dashboard + six-day weather

**Prepared:** 2026-10-11  
**GitHub branch:** `phase8a1-static-prototype`  
**Status:** Code changes committed and source/GitHub readback checks PASS. **Arduino full build, actual on-panel integration and 30-minute cadence hardware verification PENDING.** `main` is not modified.

## Scope

Phase 8C-1 proved verified Open-Meteo `forecast_days=7` / `futureDays[6]` acquisition on ESP32-S3; 8C-2 proved a single real six-column 32×32/17px/14px forecast frame fits on CrowPanel. 8C-3 connects that **same validated data and same user-approved renderer** into the existing normal Phase 7 application. Do not add a new service, network listener, independent weather timer or E-paper refresh owner.

The user requested multi-hour/overnight E-paper stability work only **at the final Phase 8 gate** after Dashboard functionality is complete. This test is a short integration check, not that long-duration test.

## Opt-in compatibility and feature isolation

For this hardware gate, **both** local flags are required:

```cpp
#define EDP_PHASE8B5B_APP_TEST 1
#define EDP_PHASE8C3_APP_TEST 1
```

`EDP_PHASE8B5B_APP_TEST` selects the normal Phase 7 app with live 8B Dashboard (including WEATHER/MARKETS navigation, DETAIL menu, DISPLAY CLEAN, moon standby/deep sleep and button wake). `EDP_PHASE8C3_APP_TEST` switches on **real six-day forecast integration within that app**. A compile-time `#error` prevents running 8C-3 without 8B-5B or simultaneously with either isolated 8C-1/8C-2 test. Disable every older preview/test flag and retain weather coordinates, Wi-Fi SSID/password solely in ignored local `config.h`.

When the 8C-3 flag is **off**, the previous accepted normal 8B-5B app still uses `WeatherService::fetchLatest()` (the 2-day endpoint) and the lower live region still truthfully displays `6-DAY FORECAST UNAVAILABLE`. The standalone 8C-1 Serial only, 8C-2 one-shot forecast typography test and static 8A mocks are unchanged.

## Data/refresh flow

```text
Existing WiFiManager + TimeService
       |
Production weatherPollDue() 30-minute gate
       |
WeatherService::fetchLatestSixDay()
  -- original SecureHttpClient and verified Open-Meteo root
  -- same current/hours/today/tomorrow plus six futureDates
       |
Production weather widget staging / Weather page snapshot
       |
stagePhase8LiveDashboardFromServices()
  calendar + BTC/ETH/HYPE prices + current weather + RSSI
  + Phase8SixDayPresentation::fromSnapshot(futureDays[6])
       |
Phase8DashboardVisualState::stage()
  compare bounded rendered text + immutable bitmap pointers
       |
Normal Phase 7 page/navigation/DETAIL gating
       |
One app-owned framebuffer render + E-paper partial/maintenance
       |
FrameBuffer copy + success-only visual/legacy commit
```

There is **one weather HTTP poll** and **one shared Dashboard refresh owner**. `Phase8DashboardFrame.forecast` contains six owned labels and summaries plus immutable 32×32 asset pointers (no borrowed API String lifetime). `Phase8SixDayPresentation::equal()` compares availability, weekday, summary and bitmap: provider raw decimal differences that round to the same native 14px text or WMO codes that map to the same icon do not on their own dirty the visible Dashboard. Conversely **a six-day change by itself** must dirty the Dashboard even if current weather/clock/raw widget bits are unchanged.

**Failure paths:** An invalid HTTPS/json response leaves `WeatherService::lastValidSnapshot()` intact. A new structurally valid response with a daily presentation that cannot fit the strict 14px width is *not* committed to the visible forecast; the existing presented six days remain, or the live UI continues to show `6-DAY FORECAST UNAVAILABLE` on first boot with no valid forecast. The current weather and original Weather page retain their existing accepted behavior. Do not insert fake daily values or truncate minus signs/digits.

**Physical update state:** Normal partial and maintenance, PAGE return to Dashboard and DETAIL `DISPLAY CLEAN` all use `Dashboard::renderLive(phase8LiveVisual.latest())` through the **same approved** 8C-2 compositor. Only after successful `displayPartialFrame()` / `maintenanceRefresh()` is `previousFrameBuffer` synchronized and `commitApplicationDashboard()` permitted. While DETAIL is visible, background data can stage without painting over the menu. No periodic full refresh or SSD1683 driver changes.

## Test on the CrowPanel

Pull latest `phase8a1-static-prototype`. In **ignored local** `firmware/CrowPanelDashboard/config.h`, keep previous functioning credentials and weather coordinates, and activate exactly:

```cpp
// #define EDP_PHASE8A1_PREVIEW 1
// #define EDP_PHASE8_FONT_TEST 1
// #define EDP_PHASE8_WEATHER_ICON_TEST 1
// #define EDP_PHASE8B2_CLOCK_TEST 1
// #define EDP_PHASE8B3_MARKET_TEST 1
// #define EDP_PHASE8B4_WEATHER_TEST 1
// #define EDP_PHASE8B5A_WIFI_TEST 1
// #define EDP_PHASE8C1_DAILY_TEST 1
// #define EDP_PHASE8C2_DISPLAY_TEST 1

#define EDP_PHASE8B5B_APP_TEST 1
#define EDP_PHASE8C3_APP_TEST 1
```

Compile/upload `firmware/CrowPanelDashboard/CrowPanelDashboard.ino` via Arduino IDE. Open Serial Monitor at **115200 baud**. On boot, first see the WAITING/--/NO DATA/unavailable content until Wi-Fi, NTP and first valid live weather/market fetches arrive. The first validated forecast (if formatter accepts it) should replace **`6-DAY FORECAST UNAVAILABLE`** with six **real** tomorrow-through-+6 columns. The same NTP HH:MM, date, BTC/ETH/HYPE perpetual prices, current condition and Wi-Fi signal still update.

Expected diagnostic lines (actual order/values vary):

```text
PASS: Phase 8B-5B owned state and presentation self-checks.
EDP Phase 8C-3: COMPLETE LIVE DASHBOARD + SIX REAL DAYS.
8C-3: seven daily dates on existing 30-minute poll.
PASS: 8C-3 six-day/date/visible-state self-checks.
Production weather poll starting...
8C-3: six future daily records validated for Dashboard staging.
8C-3: six-day forecast visual change staged.
Live dashboard flush dirty: ...
```

The firmware continues to use its original application log `Weather visible state unchanged` for **the legacy current-weather widget**; this does **not** imply that the six-day forecast is unchanged, as forecast presentation diff is evaluated independently in the owned live Dashboard frame.

### Short acceptance checklist (do before any multi-hour endurance)

- [ ] **Build/boot:** ESP32-S3 sketch compiles and uploads with only the two app flags above; Serial self-checks including 8C-3 PASS.
- [ ] **All fields:** After connecting, HH:MM and weekday/date, BTC/ETH/HYPE prices, actual Open-Meteo current weather + day/night icon and Wi-Fi RSSI all remain live.
- [ ] **Six daily columns:** The lower `UNAVAILABLE` caption disappears after first valid seven-day fetch and six date-appropriate weekdays/icons/max-min C/maximum precipitation values display without collision.
- [ ] **No duplicate poll owner:** Weather HTTPS stays on the **existing 30-minute cadence**; no additional independent 8C-3 timer, Binance data still 60 sec.
- [ ] **Coalescer:** Forecast and clock/market/Wi-Fi changes appear in one normal Dashboard physical update if their staged changes coincide. Same visible daily content should not cause an extra E-paper refresh (note: normal minute clock changes **do** refresh).
- [ ] **Weather and Markets pages:** UP/DOWN navigation works and Weather current/tomorrow + hourly outlook and Markets last-valid data still display properly; returning Dashboard shows newest staged daily values.
- [ ] **DETAIL / DISPLAY CLEAN:** Menu remains visible while updates stage; DISPLAY CLEAN invokes original verified maintenance sequence and redraws the **complete modern Dashboard including six real forecasts**; subsequent minute partials remain crisp.
- [ ] **STANDBY / Deep Sleep:** Small moon, radio-off standby and MENU/EXIT wake still restore full live page and data.
- [ ] **Normal recovery:** If Wi-Fi is temporarily interrupted, last-valid market/current/daily displays remain while reconnecting; no fictitious fallback data. This was previously accepted in 8B-6B but now should be checked briefly with the complete Dashboard if convenient.

**Do not mark unobserved fault injection, exact 30-minute scheduler timing, midnight daily list rollover, negative temperature extremes or several hours of ghosting as PASS based on a general normal screen report.** The full-feature **Phase 8 final long-duration/endurance** gate remains deferred, per user request.

## Known limitations / deferred questions

- The six-day display intentionally uses daytime-representative condition artwork for daily weather codes (daily forecast contains no `isDay`). The upper current weather retains proper day/night selection.
- Temperatures are rounded to the nearest integer for narrow 14px summaries but full float source values are not altered.
- Very long valid-format negative temperature strings may exceed the 128px column; formatter preserves the last good six-day display rather than clipping. A future deliberate design choice could introduce compact glyph formatting.
- Date/weekday/forecast boundaries and endurance through real local midnight remain for final Phase 8 testing; daily values ordinarily advance with the subsequent verified provider response. Do not change NTP/timezone or introduce arbitrary E-paper full refresh to simulate this.
- The original 8B-5B mode is independently accessible by turning off the 8C-3 flag. Production `main` is unchanged. Final feature-branch promotion is a separate decision.

**Phase 8C-3 implementation code checkpoint complete; physical acceptance pending.**
