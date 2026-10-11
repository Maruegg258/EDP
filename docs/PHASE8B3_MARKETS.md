# Phase 8B-3 — Binance USDⓈ-M live-price slice

**Date:** 2026-10-11  
**Branch:** `phase8a1-static-prototype`  
**Status:** Phase 8B-3 isolated NTP + live Binance perpetual Dashboard is **accepted after a user-reported successful CrowPanel test (2026-10-11)**. Earlier code-level checks passed. Extended failure, rollover and endurance scenarios remain unverified.

## Approved scope

In the user-accepted 792×272 Phase 8A composition, only these live fields are now enabled by an **opt-in experiment**:

- Calendar `HH:MM` / `DD MMM` / uppercase weekday, from the already verified `TimeService`/NTP path (Phase 8B-2).
- The existing `MarketDataService` fetches `BTCUSDT`, `ETHUSDT`, `HYPEUSDT` USDⓈ-M **perpetual** prices via `https://fapi.binance.com/fapi/v2/ticker/price?symbol=...` using the pre-existing verified TLS trust anchor, retaining the full source string and last valid result per symbol.
- Visual market order in the accepted Rev-D2 layout is **ETH → BTC → HYPE**. Each label remains 17px above a centered 34px price within the original three 248px-wide columns.

The header weather condition `SUNNY`, sample temperature `26.9 C`, six forecast mock rows `25-29/ 10%`, weekday labels and all weather icons, and the graphic Wi-Fi signal-strength icon remain **STATIC SAMPLES** during this mode. It intentionally does **not** call `WeatherService`; visible Wi-Fi strength is not yet RSSI-connected.

**Do not leave this test mode enabled and present the simulated weather as live information.** Normal Phase 7 production paths remain unchanged; real full-page integration will happen in Phase 8B-5, and six real daily forecasts require Phase 8C.

## Presentation data and width policy

New pure module `Phase8MarketPresentation.h/.cpp` owns three `char[11]` display strings in `Phase8MarketPrices` (10 visible ASCII glyphs + terminator); the text is never a dangling pointer into temporary `MarketPriceValue` objects or JSON buffers. Existing `DashboardFont34` advances 22px per character, giving **220px maximum** inside the 248px cell and leaving ≥14px of horizontal margin on each side for the longest display value.

Source data remains an unmodified decimal string in `MarketDataService`. The **display** is rounded to an explicitly documented policy:

| Market | Preferred max decimal places | Examples |
| --- | ---: | --- |
| BTCUSDT | 1 | `82750.10000000` → `82750.1` |
| ETHUSDT | 2 | `2493.56000000` → `2493.56` |
| HYPEUSDT | 3 | `84.15400000` → `84.154` |

For very large magnitudes, decrease decimal places until both integer digits and any carry fit **≤10 characters**; round half-up using string digits, never binary-float reformatting, scientific notation or truncation of the integer part. If a valid market result cannot fit even as an integer, use `--` rather than overflow/crop. Invalid, negative or non-finite-looking input is rejected. For tiny positive values below a displayed tick, half-up rounding may display zero; this is an accepted limitation of the fixed-precision experimental formatting and needs review if extremely small-price pairs are added later.

The **full Binance price is not modified**; only the 34px visual approximation uses this rounding.

If HTTPS fetch fails, use `MarketDataService::lastValidValue(symbol)` when available. The test initially shows `--` until each first successful fetch. A rounded visible value that remains identical generates **no display update**, even if source string changes by less than one display tick.

## How to compile / upload for hardware review

1. Pull the latest `phase8a1-static-prototype` branch.
2. In your ignored local `config.h`, keep your existing working `WIFI_SSID` and `WIFI_PASSWORD`, and enable **only**:

   ```cpp
   // #define EDP_PHASE8A1_PREVIEW 1
   // #define EDP_PHASE8_FONT_TEST 1
   // #define EDP_PHASE8_WEATHER_ICON_TEST 1
   // #define EDP_PHASE8B2_CLOCK_TEST 1
   #define EDP_PHASE8B3_MARKET_TEST 1
   ```

3. Arduino IDE: compile/upload `CrowPanelDashboard.ino`; open Serial Monitor at 115200 baud.
4. Boot briefly displays `WAITING` and `--` on all three markets using the **verified maintenanceRefresh()** initial pattern.
5. Once NTP is synchronized, the three Binance futures symbols are fetched at most once per 60 seconds when Wi-Fi is connected. Clock/calendar and all three prices are compared to the physically displayed values, rendered into one full framebuffer, and physically refreshed once using the existing `begin → restoreFrameStateForPartial(previousFrameBuffer) → displayPartialFrame → copy previousFrameBuffer → sleep` application sequence. This is **a full-canvas composition with partial physical update**, not a newly invented full refresh cycle.
6. When a fetch fails, the Service's last valid value is used without repainting a failure string over it; never fetches weather or changes the mock Wi-Fi icon. Data is read-only via Binance public HTTPS—no API key, wallet, order privileges or user account endpoint.
7. Switch back to the prior 8B-2 or Phase 8A static flag to confirm old code paths still render identically.

Expected serial diagnostics (sample only, not actual live prices):

```text
EDP Phase 8B-3: NTP + BINANCE USDT PERPETUAL PRICES
PASS: 8B-3 calendar and price formatting self-checks.
PASS: 8B-3 WAITING / -- placeholders displayed.
8B-3: polling three existing USD-M perpetual symbols.
Market fetched BTCUSDT raw=82750.10000000 shown=82750.1
Market fetched ETHUSDT raw=2493.56000000 shown=2493.56
Market fetched HYPEUSDT raw=84.15400000 shown=84.154
PASS: 8B-3 coalesced partial refresh (clock=changed, prices=changed).
```

## Code boundaries and verification

- [x] Original user-approved Phase 8A static `render()` keeps its original numbers and weather icons.
- [x] Previous Phase 8B-2 `renderWithCalendar()` retains mock prices; do not force a new price service call in that accepted mode.
- [x] New `renderWithCalendarAndMarkets()` shares the same Phase 8A layout and glyph assets; price widths and native 220px bounds checked.
- [x] Own 3×11-char market prices; 60-second polling, already verified TLS/HTTPS and last-valid per-symbol preservation reused without reimplementation.
- [x] Bootstrap `WAITING` and `--` placeholders; only commit displayed calendar+prices after physical refresh succeeds; aggregate all 3 symbol polls before one visible redraw.
- [x] Mutually exclusive four prior preview/diagnostic flags + 8B-3 mode.
- [x] C++11 host-compiled equivalent of `Phase8MarketPresentation.cpp` (using `-Wall -Wextra -Werror -pedantic`) passed internal formatting and malformed/overflow input self-checks; actual ESP32 toolchain compilation **has not** been executed.
- [ ] ESP32-S3 Arduino IDE compile/upload and boot self-check.
- [ ] Verify real ETH/BTC/HYPE **perpetual** values and change after ≥60 seconds, along with correct NTP date/time.
- [ ] Check unchanged-data suppression, stable labels/market row, controller seam and absence of ghosting after 2–3 updates.
- [ ] If convenient, test Wi-Fi loss/reconnection (or allow this to be covered under 8B-6).
- [ ] On user approval, mark 8B-3 hardware gate PASS and proceed to 8B-4 live current weather.

**Scope warning:** The normal `Dashboard`, `PageRenderer`, `DashboardUpdateCoalescer`, `WeatherService`, `MarketDataService`, `CrowEPD579`, secure HTTP client, existing Phase 7 page buttons and deep-sleep actions were not modified in Phase 8B-3. No change to the E-paper maintenance-refresh sequence or periodic full-refresh policy.

## Phase 8B-3 — Hardware acceptance (2026-10-11)

**Actual device feedback:** The user reports **「實際測試正常」** after the Phase 8B-3 real-price test. Mark the combined **NTP date/time/weekday plus live ETH/BTC/HYPE prices** preview **PASS for this device checkpoint**, following separate prior 8B-2 calendar acceptance. The 34px ETH/BTC/HYPE row and already accepted calendar/header layout remain the feature-branch baseline.

**Evidence boundary:** The general successful test confirms the tested firmware/display behavior, not every suggested optional test. The user did not separately provide a photo, serial timestamps, manually induced HTTPS/Wi-Fi failure/recovery, no-change suppression logs, extreme-value data, midnight transition or long-duration ghosting record. Preserve those as later regression checks rather than marking them all individually verified.

**Next step:** Phase 8B-4 current weather: re-use the validated `WeatherService` snapshot, map live condition + day/night with `WeatherIconSelector`, format the actual temperature/label within the accepted 32px/17px/14px header, and track visual changes with the same application-owned coalesced refresh in an isolated test mode. Retain live calendar and all three prices. Six-day forecast values and Wi-Fi strength graphic stay **explicit mock samples** pending Phase 8C and 8B-5. Do not change normal Phase 7 `Dashboard`, `PageRenderer`, `DashboardUpdateCoalescer`, TLS verification, or SSD1683 refresh on the basis of this acceptance alone.

**Git record:** Documentation-only hardware acceptance. No firmware bytes are changed by this checkpoint.
