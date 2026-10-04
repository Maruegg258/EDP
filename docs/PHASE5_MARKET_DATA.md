# Phase 5 market-data baseline

**Status:** Phase 5A, Phase 5B-1, and Phase 5B-2 hardware verified; Phase 5B-3 production cleanup implemented and awaiting final hardware regression.

**Reviewed:** 2026-10-04

This document freezes the initial data-source and TLS contract for Phase 5 before firmware implementation begins.

## Scope

Phase 5 will provide the dashboard with the latest traded prices for three Binance USDⓈ-M perpetual-futures contracts:

| Dashboard asset | Binance symbol | Contract type | Quote / margin asset |
| --- | --- | --- | --- |
| BTC | `BTCUSDT` | `PERPETUAL` | USDT |
| ETH | `ETHUSDT` | `PERPETUAL` | USDT |
| HYPE | `HYPEUSDT` | `PERPETUAL` | USDT |

On 2026-10-04, Binance USDⓈ-M `exchangeInfo` reported all three symbols as `PERPETUAL` and `TRADING`.

That upstream status is current exchange metadata, not a permanent firmware assumption. A future delisting or contract change must be handled as unavailable market data rather than silently substituting another market.

## Provider and endpoint

Production API host:

```text
https://fapi.binance.com
```

Price endpoint:

```text
GET /fapi/v2/ticker/price?symbol=<SYMBOL>
```

The three initial requests are therefore:

```text
GET https://fapi.binance.com/fapi/v2/ticker/price?symbol=BTCUSDT
GET https://fapi.binance.com/fapi/v2/ticker/price?symbol=ETHUSDT
GET https://fapi.binance.com/fapi/v2/ticker/price?symbol=HYPEUSDT
```

Binance documents this endpoint as the USDⓈ-M Futures **Symbol Price Ticker V2** endpoint for the latest price of a symbol. A single-symbol request has request weight 1.

Official references:

- https://developers.binance.com/en/docs/catalog/core-trading-derivatives-trading-usd-s-m-futures/api/rest-api/market-data
- `GET /fapi/v2/ticker/price`
- `GET /fapi/v1/exchangeInfo`

## Price meaning

The value displayed by the Phase 5 crypto widgets is the **latest traded price (Last Price)** of the selected USDⓈ-M perpetual contract.

Phase 5 does not use mark price as the primary displayed price.

Mark price, index price, funding rate, and other derivatives data may be added later as separate service fields if the dashboard needs them. They must not silently replace Last Price.

## Why the earlier test endpoint is not used

Earlier bring-up work successfully tested public Binance data through `data-api.binance.vision`.

That endpoint was useful for proving ESP32 Wi-Fi/HTTP access, but it is not the Phase 5 production contract because the dashboard requirement is explicitly perpetual-futures pricing.

Phase 5 therefore uses the USDⓈ-M Futures host `fapi.binance.com`.

## Request strategy

The firmware should request the three configured symbols individually instead of downloading the no-symbol response containing the full Futures symbol universe.

Initial request pattern:

```text
BTCUSDT -> one small response
ETHUSDT -> one small response
HYPEUSDT -> one small response
```

This costs three low-weight requests per polling cycle but keeps ESP32 network buffers and JSON parsing small and predictable. With the planned 1-5 minute polling cadence, this is preferable to downloading the complete ticker set.

The symbol list belongs to the market-data service/configuration boundary. UI widgets must not construct Binance URLs themselves.

## Successful-value acceptance

A fetched value should replace the current market value only when all required checks succeed:

1. HTTPS/TLS connection succeeds.
2. HTTP status is successful.
3. JSON parses successfully.
4. The returned `symbol` exactly matches the requested symbol.
5. The returned `price` parses as a finite positive numeric value.

The response `time` field may also be retained for diagnostics or freshness tracking.

A failed request must not replace a valid displayed price with `0`, an empty string, malformed data, or a partially parsed value.

## Failure behavior

Network, TLS, HTTP, JSON, or validation failure should be represented as a data-fetch failure.

The service should:

- preserve the last valid market value when one exists
- report failure/staleness separately from the numeric value
- avoid triggering a display change solely because a failed fetch returned no price
- retry according to the application polling/retry policy
- never disable TLS verification in order to recover data

Exact retry/backoff timing can be introduced incrementally during Phase 5 implementation.

## HTTPS and certificate validation

Phase 5 production HTTPS must use certificate validation.

Planned implementation model:

```text
Wi-Fi connected
        |
        v
TimeService synchronized
        |
        v
NetworkClientSecure + trusted CA
        |
        v
HTTPS request to fapi.binance.com
        |
        v
MarketDataService validation
```

The first certificate-validated market request must wait until the device has trustworthy synchronized time.

Initial trust anchor reviewed for Phase 5:

```text
DigiCert Global Root G2
```

At review time, Binance's `*.binance.com` certificate path used `GeoTrust TLS RSA CA G1`, which DigiCert documents as issued by `DigiCert Global Root G2`.

The root certificate is public trust material and may be committed with the firmware. Private keys, client certificates containing secrets, API keys, tokens, and account credentials must not be committed.

Forbidden production behavior:

```cpp
client.setInsecure();
```

Leaf-certificate fingerprint pinning is also not the initial design because routine leaf-certificate rotation would create unnecessary firmware outages.

If Binance changes certificate authorities in the future, the correct recovery path is to review and update the trust configuration, not to fall back to insecure TLS.

## Authentication and permissions

The selected market-data endpoint is public market data.

Phase 5 does **not** require:

- Binance API key
- Binance secret key
- signed requests
- trading permission
- account access
- withdrawal permission

No high-privilege Binance credential should be placed in this dashboard firmware.

## Architectural boundary

The intended dependency direction is:

```text
WiFiManager / TimeService
          |
          v
Secure HTTPS network layer
          |
          v
MarketDataService
          |
          v
DashboardUpdateCoalescer
          |
          v
BTC / ETH / HYPE widget state
```

The market-data service must not perform E-paper refreshes.

The display driver must not know about Binance, HTTPS, JSON, symbols, or polling intervals.

## Evidence status

**Confirmed from Binance API metadata/documentation on 2026-10-04:**

- USDⓈ-M Futures host is `fapi.binance.com`
- Symbol Price Ticker V2 is `GET /fapi/v2/ticker/price`
- `BTCUSDT`, `ETHUSDT`, and `HYPEUSDT` were listed as `PERPETUAL` and `TRADING`
- single-symbol Symbol Price Ticker V2 requests have request weight 1

**Confirmed from DigiCert documentation during the same review:**

- `GeoTrust TLS RSA CA G1` is issued by `DigiCert Global Root G2`
- `DigiCert Global Root G2` is valid until 2038-01-15

**Hardware-verified on the development CrowPanel (2026-10-04):**

- the device waits for synchronized system time before the Phase 5A-1 HTTPS probe
- TLS validation succeeds against `fapi.binance.com` using the configured DigiCert Global Root G2 trust anchor
- the BTC Symbol Price Ticker V2 request returns HTTP 200
- the response body contains the expected `BTCUSDT` ticker payload
- the existing Phase 4 Clock and Wi-Fi behavior remains normal while the HTTPS probe is present
- no insecure TLS fallback is used

**Hardware-verified on the development CrowPanel (2026-10-04):**

- the device waits for synchronized system time before the Phase 5A-1 HTTPS probe
- TLS validation succeeds against `fapi.binance.com` using the configured DigiCert Global Root G2 trust anchor
- the BTC Symbol Price Ticker V2 request returns HTTP 200
- the response body contains the expected `BTCUSDT` ticker payload
- `MarketDataService` parses and validates the BTC `symbol`, positive finite `price`, and source `time`
- an intentional invalid-symbol request fails as expected
- the previously valid BTC snapshot remains unchanged after the failed request
- the existing Phase 4 Clock and Wi-Fi behavior remains normal
- crypto widgets remain unchanged during the Serial-only service tests
- no insecure TLS fallback is used

**Hardware-verified live UI integration (2026-10-04):**

- validated BTC/ETH/HYPE prices stage into the matching Crypto widget state through `DashboardUpdateCoalescer`
- startup placeholders are replaced by live perpetual prices after the first successful market poll
- live Crypto updates use the existing verified partial-refresh path
- Clock/Wi-Fi behavior and display clarity remain normal with market data enabled
- market polling remains separate from direct display-driver control

**Hardware-verified resilience behavior (2026-10-04):**

- re-staging identical live Crypto values produces no dirty state and no unnecessary physical refresh
- simulated one-symbol unavailability preserves the displayed and service last-valid value
- unaffected symbols continue independently
- normal polling recovers after the unavailable cycle

**Deferred to Phase 9 reliability hardening:**

- prolonged market-data outage thresholds
- explicit data-age/stale indication if later judged useful
- long-duration API outage/recovery testing

The temporary Phase 5B-2 diagnostic injection must still be removed and the production path re-verified before Phase 5 is closed.

## Phase 5A-1 implementation

**Status:** PASS on real hardware (2026-10-04).

Phase 5A-1 adds:

- `SecureHttpClient.h/.cpp` as a network-layer HTTPS wrapper
- `TlsTrustAnchors.h` containing the public DigiCert Global Root G2 PEM trust anchor
- `NetworkClientSecure::setCACert(...)` certificate validation
- explicit HTTP connect/read timeouts
- one BTC test URL: `https://fapi.binance.com/fapi/v2/ticker/price?symbol=BTCUSDT`
- a one-shot application probe that runs only after Wi-Fi is connected and `TimeService` reports synchronized time
- Serial-only reporting; BTC/ETH/HYPE widget state is not changed by this checkpoint

The existing Phase 4 Clock/Wi-Fi UI flow remains active. The HTTPS probe is deliberately not a market-data service yet.

Expected successful Serial sequence includes:

```text
Phase 5A-1 HTTPS probe starting...
TLS mode: DigiCert Global Root G2 validation
HTTP status: 200
Response body:
{"symbol":"BTCUSDT","price":"...","time":...}
PASS: certificate-validating HTTPS request completed successfully.
Phase 5A-1 does not stage market data into the dashboard.
```

Hardware verification confirmed:

1. Firmware compiles and uploads on the development CrowPanel.
2. NTP reaches `SYNCHRONIZED` before the HTTPS probe starts.
3. The probe returns HTTP 200 from the configured Binance Futures endpoint.
4. The response body contains the expected BTC ticker payload.
5. Existing minute Clock and Wi-Fi behavior remain normal.
6. No `setInsecure()` is present or used.

Phase 5A-1 is therefore complete.

## Phase 5A-2 implementation

**Status:** PASS on real hardware (2026-10-04).

Phase 5A-2 adds a dedicated `MarketDataService` with the following boundary:

```text
SecureHttpClient
       |
       v
MarketDataService
       |
       v
validated MarketPriceValue
```

The service owns Binance ticker URL construction and validation. The application no longer parses the raw HTTPS body.

Initial Phase 5A-2 scope remains BTC-only:

```text
BTCUSDT
```

For the fixed, small Symbol Price Ticker V2 payload, this checkpoint uses a project-owned narrow parser rather than adding an external JSON-library dependency. It accepts fields independent of ordering and validates only the contract required by this service.

A successful value is committed only after:

1. HTTPS succeeds.
2. `symbol` exists as a JSON string and exactly matches the requested symbol.
3. `price` exists as a JSON string and parses completely as a finite number greater than zero.
4. Optional `time`, when present, is an unsigned integer.

The service stores its own fixed-capacity `MarketPriceValue` snapshot. Parsing is performed into a temporary value first, so a failed HTTP request or failed validation does not overwrite the existing last-valid snapshot.

The Phase 5A-2 application probe performs:

1. one real `BTCUSDT` fetch through the hardware-verified TLS path
2. Serial output of parsed symbol, price, and source time
3. one intentional invalid-symbol request
4. a comparison proving the valid BTC snapshot remains unchanged after that expected failure

This is still Serial-only. No BTC value is staged into `DashboardUpdateCoalescer`, and ETH/HYPE are not fetched yet.

Expected successful Serial sequence includes:

```text
Phase 5A-2 MarketDataService probe starting...
Requested symbol: BTCUSDT
Parsed BTC symbol: BTCUSDT
Parsed BTC price: ...
Parsed BTC source time: ...
Testing last-valid preservation with an intentional invalid-symbol request...
Expected failure: HTTPS fetch failed: HTTP status ...
Preserved BTC symbol: BTCUSDT
Preserved BTC price: ...
PASS: MarketDataService parsed BTC and preserved last-valid data after failure.
Phase 5A-2 remains Serial-only; dashboard crypto widgets are unchanged.
```

Hardware verification confirmed:

1. Firmware compiles and uploads.
2. The BTC request runs only after Wi-Fi and synchronized time are available.
3. Parsed `symbol` is exactly `BTCUSDT`.
4. Parsed `price` is a valid positive value.
5. Source `time` is parsed successfully.
6. The intentional invalid-symbol request fails as expected.
7. The previously valid BTC snapshot remains unchanged after that failure.
8. Existing Clock/Wi-Fi dashboard behavior remains normal.
9. Crypto widgets remain unchanged in this checkpoint.

Phase 5A-2 is therefore complete.

## Phase 5A-3 implementation

**Status:** PASS on real hardware (2026-10-04).

Phase 5A-3 expands `MarketDataService` to three independent configured market slots:

```text
BTCUSDT
ETHUSDT
HYPEUSDT
```

Each slot owns:

- its configured symbol
- its own last-valid `MarketPriceValue`
- a valid/not-valid flag
- its most recent fetch/validation error

A successful fetch commits only to the matching symbol slot. A failed fetch leaves that slot's previous valid value untouched and cannot overwrite another symbol's state.

The application introduces a first real polling cadence:

```text
60 seconds
```

This is inside the project's initial 1-5 minute market-data guideline. The poll runs only while Wi-Fi is connected and system time is synchronized.

The market poll is deliberately invoked after the existing Clock/Wi-Fi dashboard flush. The goal is to prevent a slow HTTPS request from taking priority over an already-due minute display update.

Each polling cycle:

1. fetches `BTCUSDT`
2. fetches `ETHUSDT`
3. fetches `HYPEUSDT`
4. prints each successful parsed value through Serial
5. prints a per-symbol error on failure
6. prints the preserved last-valid value for a failed symbol when one exists
7. reports how many of the three symbols updated successfully
8. verifies whether all three independent slots contain valid data

No crypto widget is updated in Phase 5A-3.

Expected healthy Serial output includes:

```text
Phase 5A-3 three-symbol market poll starting...
Fetching BTCUSDT...
Updated symbol: BTCUSDT
Updated price: ...
Fetching ETHUSDT...
Updated symbol: ETHUSDT
Updated price: ...
Fetching HYPEUSDT...
Updated symbol: HYPEUSDT
Updated price: ...
Market poll result: 3/3 symbols updated.
PASS: BTC/ETH/HYPE have independent last-valid market slots.
Phase 5A-3 remains Serial-only; dashboard crypto widgets are unchanged.
```

Hardware verification confirmed:

1. Firmware compiles and uploads.
2. The first market cycle starts only after Wi-Fi and NTP synchronization.
3. `BTCUSDT`, `ETHUSDT`, and `HYPEUSDT` all parse successfully into distinct service slots.
4. Serial reports valid positive prices and source times for all three symbols.
5. Repeated polling occurs at approximately 60-second application intervals.
6. The three stored last-valid values remain associated with their matching symbols across repeated polls.
7. Existing Clock/Wi-Fi display behavior remains normal.
8. Crypto widgets remain unchanged, as intended for the Serial-only checkpoint.

The multi-symbol failure path was not deliberately forced during this successful Phase 5A-3 run. The service is designed to retain a failed symbol's prior valid slot, and BTC last-valid preservation was already hardware-verified in Phase 5A-2. A broader induced-outage/per-symbol failure test remains appropriate for later reliability hardening.

Phase 5A-3 is therefore complete.

## Phase 5B-1 implementation

**Status:** PASS on real hardware (2026-10-04).

Phase 5B-1 keeps the verified network/service layers unchanged and adds only the application-to-UI integration:

```text
MarketDataService last-valid value
            |
            v
CryptoWidgetState { price }
            |
            v
DashboardUpdateCoalescer
      |       |       |
     BTC     ETH     HYPE
            |
            v
existing application flush
            |
            v
verified E-paper partial-refresh path
```

The symbol-to-widget mapping lives in the application layer:

- `BTCUSDT` -> `stageBtc()`
- `ETHUSDT` -> `stageEth()`
- `HYPEUSDT` -> `stageHype()`

Neither `MarketDataService` nor `SecureHttpClient` knows about widgets, dirty masks, framebuffers, or E-paper refreshes.

### Startup behavior

The initial dashboard baseline now uses:

```text
BTC  --
ETH  --
HYPE --
```

This prevents Phase 3 regression-test prices from appearing as if they were live market data before the first successful poll.

### Successful poll behavior

For each successfully validated symbol, the application stages the service-owned price string into the matching Crypto widget state.

The existing durable `DashboardStateSnapshot` copies the price into its own 24-byte BTC/ETH/HYPE buffers, so the pending UI state does not depend on the lifetime of a temporary pointer.

The existing content-based comparison then determines whether the visible price actually changed:

- changed price -> matching crypto dirty bit is set
- unchanged price -> no new crypto dirty bit
- multiple changed symbols -> dirty bits accumulate in one pending state

### Failure behavior

A failed market fetch stages no replacement value.

Therefore:

- no zero/empty/error string is sent to a Crypto widget
- a previously displayed valid price remains visible
- a startup symbol with no valid value remains `--`
- one symbol's failure does not prevent other successful symbols from being staged

### Polling versus display refresh

The verified 60-second market polling cadence is unchanged.

The application keeps the Phase 5A-3 ordering:

1. stage due Clock/Wi-Fi changes
2. flush existing pending dashboard state
3. perform the due market poll
4. stage successful market values into the coalescer
5. return to the main loop
6. on the following loop, stage any newly due system changes and use the existing application flush

The market-data layer never triggers an E-paper refresh directly. This preserves the separation between data retrieval and physical refresh and leaves a coalescing opportunity before the pending market state is physically displayed.

Expected first successful live update includes Serial output similar to:

```text
Phase 5B-1 live Crypto widget market poll starting...
Fetching BTCUSDT...
Updated symbol: BTCUSDT
Updated price: ...
Staged live BTCUSDT; pending dirty: ... [BTC]
Fetching ETHUSDT...
Updated symbol: ETHUSDT
Updated price: ...
Staged live ETHUSDT; pending dirty: ... [BTC|ETH]
Fetching HYPEUSDT...
Updated symbol: HYPEUSDT
Updated price: ...
Staged live HYPEUSDT; pending dirty: ... [BTC|ETH|HYPE]
Market staging result: 3/3 symbols accepted by dashboard state.
No display refresh is triggered directly by MarketDataService.
```

The next application loop should then report a live-dashboard flush containing the accumulated Crypto dirty bits and the panel should replace the three `--` placeholders with live perpetual prices.

Hardware verification confirmed:

1. Firmware compiles and uploads.
2. Startup shows `--` for BTC, ETH, and HYPE before the first valid market update.
3. The first successful poll stages and displays live BTC/ETH/HYPE perpetual prices.
4. The displayed prices match the validated market values reported through Serial.
5. Crypto updates pass through `DashboardUpdateCoalescer`; `MarketDataService` performs no display operation.
6. Multiple live Crypto changes accumulate through the existing dirty/coalescing path and are rendered through the verified partial-refresh flow.
7. Existing Clock/Wi-Fi behavior remains normal.
8. Text remains clear with no observed refresh-quality regression.
9. The 60-second market polling cadence remains in place independently of the display driver.

The normal live-data path is therefore hardware-verified and Phase 5B-1 is complete.

The following behaviors are deliberately left for the next checkpoint rather than being inferred from a successful normal run:

- an explicitly unchanged market price producing no Crypto dirty bit / no unnecessary physical refresh
- an intentionally induced fetch failure while live values are already displayed
- recovery after that failure
- whether a stale-data/status indicator is necessary for prolonged outages

## Phase 5B-2 implementation

**Status:** PASS on real hardware (2026-10-04).

Phase 5B-2 is a diagnostic checkpoint around the already hardware-verified live market pipeline. It deliberately does not change:

- `SecureHttpClient`
- `MarketDataService`
- `DashboardUpdateCoalescer`
- the E-paper driver
- the 60-second production polling interval

Instead, the application runs a one-shot state machine after the first valid BTC/ETH/HYPE prices have been physically displayed.

### Diagnostic 1 — unchanged-price skip

The application re-stages the exact current service last-valid values for BTC, ETH, and HYPE.

Required result:

```text
Unchanged-price pending dirty: 0x0 [NONE]
PASS: unchanged Crypto prices produce dirty NONE and no physical refresh.
```

This verifies the live-data path, not only the older synthetic Phase 3 self-check.

### Diagnostic 2 — one-symbol unavailable hold

For one diagnostic polling cycle only, the application treats `ETHUSDT` as unavailable by deliberately skipping its fetch/stage operation.

This is explicitly an **application-level availability simulation**, not a claim that the transport failed. The real HTTP failure + last-valid preservation behavior was already hardware-verified in Phase 5A-2.

During this simulated unavailable cycle:

- BTC fetch/staging continues normally
- HYPE fetch/staging continues normally
- ETH receives no replacement value
- the service's existing ETH last-valid value remains untouched
- the displayed ETH price must remain exactly equal to the pre-failure displayed value

Expected Serial markers include:

```text
DIAGNOSTIC: ETHUSDT will be treated as unavailable for this poll only.
EXPECTED DIAGNOSTIC FAILURE ETHUSDT: simulated market data unavailable; fetch skipped.
UI action: keep the previously displayed ETH price; no replacement value staged.
PASS: simulated ETH unavailability preserved the displayed and service last-valid ETH price.
```

### Diagnostic 3 — recovery

After the preservation check passes, the application immediately re-enables the normal ETH request rather than waiting another full 60 seconds.

The recovered ETH fetch must succeed, stage through the same Crypto/coalescer path, and the displayed ETH state must match the service's recovered last-valid price after the next application flush.

Expected completion markers include:

```text
DIAGNOSTIC: normal ETHUSDT fetch restored for recovery verification.
PASS: ETH market polling recovered and the dashboard state matches the recovered last-valid value.
PASS: Phase 5B-2 unchanged-price + failure-hold + recovery diagnostics complete.
```

After the one-shot diagnostic completes, normal 60-second BTC/ETH/HYPE polling continues.

### Stale-data decision

This checkpoint does not add a stale-data marker in advance.

The diagnostic answers whether short-lived failures are safely handled by preserving last-valid data. Whether prolonged outages need a visual stale/status indication remains a product decision to make after the hardware result.

Hardware verification confirmed:

1. Firmware compiles and uploads.
2. Normal live BTC/ETH/HYPE display becomes established before diagnostics run.
3. Re-staging identical live prices produces `DashboardDirty::NONE` and no pending physical refresh.
4. The one-cycle ETH unavailable simulation leaves the displayed ETH price unchanged.
5. The service's ETH last-valid value remains unchanged during the simulated unavailable cycle.
6. BTC and HYPE continue independently through that cycle.
7. Normal ETH fetching resumes successfully.
8. After recovery, displayed ETH matches the service last-valid ETH price.
9. Existing Clock/Wi-Fi behavior remains normal.
10. E-paper text remains clear with no observed refresh-quality regression.
11. Normal 60-second market polling continues after the one-shot diagnostics complete.

Phase 5B-2 is therefore complete.

### Stale-data decision

The verified behavior shows that short-lived market-data failures are safely absorbed by preserving the last valid displayed value, without injecting zero/error text or forcing unnecessary refreshes.

For Phase 5, no additional stale-data visual indicator will be added.

A prolonged outage is a different product/reliability case. Explicit age/staleness indication, outage thresholds, and long-duration recovery policy are deferred to **Phase 9 — Reliability and release hardening**, where they can be designed together with API outage tests and long-duration operation rather than adding a premature UI state here.

### Why Phase 5 is not closed yet

The current firmware still contains the temporary one-shot Phase 5B-2 diagnostic injection. That code intentionally simulates ETH unavailability once after every boot and therefore should not remain in the production application path.

## Phase 5B-3 production cleanup

**Implementation status:** committed 2026-10-04; final real-hardware regression pending.

Phase 5B-3 removes all temporary Phase 5B-2 diagnostic behavior from the production application path:

- no diagnostic state enum
- no boot-time unchanged-price re-staging test
- no simulated ETH-unavailable cycle
- no forced immediate recovery poll
- no diagnostic-only market timing overrides

The production market path is again only:

```text
Wi-Fi connected + synchronized time
            |
            v
60-second market poll
            |
            +-- BTCUSDT fetch/validate
            +-- ETHUSDT fetch/validate
            +-- HYPEUSDT fetch/validate
            |
            v
successful values -> DashboardUpdateCoalescer
failed values     -> stage nothing / preserve displayed last-valid
            |
            v
existing application flush
            |
            v
verified partial-refresh path
```

The following verified components are deliberately unchanged by this cleanup:

- `SecureHttpClient` certificate validation
- `MarketDataService` parsing, per-symbol slots, and last-valid behavior
- `DashboardUpdateCoalescer` content comparison and dirty accumulation
- 60-second market polling interval
- Clock/Wi-Fi application flow
- E-paper driver and maintenance/partial refresh sequences

Expected normal Serial behavior no longer contains `DIAGNOSTIC`, simulated failure, or recovery-test messages. A regular cycle should resemble:

```text
Phase 5B-3 production market poll starting...
Fetching BTCUSDT...
Updated symbol: BTCUSDT
...
Fetching ETHUSDT...
Updated symbol: ETHUSDT
...
Fetching HYPEUSDT...
Updated symbol: HYPEUSDT
...
Market fetch result: 3/3 symbols updated.
Market staging result: 3/3 symbols accepted by dashboard state.
Pending dashboard dirty after market poll: ...
Market-data code does not trigger E-paper refresh directly.
```

Acceptance criteria before Phase 5 can be closed:

1. Firmware compiles and uploads.
2. No Phase 5B-2 diagnostic/simulated-failure output appears after boot.
3. Startup placeholders are replaced by valid live BTC/ETH/HYPE perpetual prices.
4. Normal market polling repeats at approximately 60-second intervals.
5. Live prices continue to stage through `DashboardUpdateCoalescer`.
6. A normal fetch failure, if one occurs naturally, stages no replacement error value and preserves last-valid behavior.
7. Clock and Wi-Fi behavior remain normal.
8. E-paper refreshes remain clear with no ghosting/blur regression.
9. No insecure TLS fallback exists.
10. No diagnostic timing override remains in the production application.

If this final regression passes, Phase 5 is complete and the roadmap can move to Phase 6 — Weather.
