# Phase 5 market-data baseline

**Status:** Phase 5A-1 hardware verified; Phase 5A-2 implemented and awaiting hardware verification.

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

**Not yet hardware-verified:**

- JSON field parsing and validation inside a dedicated market-data service
- ETH/HYPE requests and three-symbol polling behavior
- last-valid-value preservation and failure/staleness handling on the device

Those items remain for later Phase 5 checkpoints.

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

**Implementation status:** committed 2026-10-04; real-hardware verification pending.

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

Acceptance criteria before Phase 5A-2 can be marked complete:

1. Firmware compiles and uploads.
2. The BTC request succeeds only after Wi-Fi and synchronized time are available.
3. Parsed `symbol` is exactly `BTCUSDT`.
4. Parsed `price` is a valid positive value.
5. Source `time` is parsed when Binance provides it.
6. The intentional invalid-symbol request fails.
7. The previously valid BTC snapshot remains unchanged after that failure.
8. Existing Clock/Wi-Fi dashboard behavior remains normal.
9. Crypto widgets remain unchanged in this checkpoint.

## Next checkpoint

After Phase 5A-2 hardware verification, extend the service incrementally to the remaining configured perpetual symbols before connecting market values to the dashboard UI.
