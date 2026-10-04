# Security baseline

## Repository policy

This repository is public. Never commit:

- Wi-Fi passwords
- API keys or tokens
- private certificates or private keys
- device-specific secrets

Use `config.h` locally. It is excluded by `.gitignore`. Commit only `config.example.h`.

Public CA certificates used only as TLS trust anchors are not secrets and may be committed to the repository.

## Network policy

The intended production firmware should have the smallest practical network surface:

- outbound Wi-Fi client only
- outbound NTP/SNTP requests only to explicitly configured time servers for clock synchronization
- outbound HTTPS requests only to explicitly configured market-data and weather endpoints
- TLS certificate validation enabled
- no `setInsecure()` in production
- no OTA service unless deliberately designed and reviewed
- no web server
- no MQTT broker connection
- no BLE / Bluetooth service
- no ESP-NOW service
- no telemetry or analytics

## Phase 5 market-data TLS policy

Phase 5 market data is read-only public data from Binance USDⓈ-M Futures.

Production host:

```text
https://fapi.binance.com
```

The dashboard does not require a Binance API key, account credential, trading permission, or other exchange secret for this data path.

The production TLS path must follow these rules:

1. Wi-Fi must be connected before a market-data request is attempted.
2. The system clock must be synchronized before the first certificate-validated HTTPS request, so certificate validity dates can be checked against a trustworthy device time.
3. HTTPS must use `NetworkClientSecure` with an explicitly configured trusted CA certificate such as `setCACert(...)`.
4. Production code must never call `setInsecure()`.
5. Leaf-certificate or fingerprint pinning is not the default trust model because normal server-certificate rotation would unnecessarily break the firmware.
6. A TLS validation failure is a market-data fetch failure. The firmware must not downgrade to insecure TLS as a fallback.
7. When TLS validation fails after an upstream certificate-chain change, preserve the last valid market value, diagnose the chain change, and update the trusted CA configuration deliberately.

Initial Phase 5 trust anchor, reviewed 2026-10-04:

```text
DigiCert Global Root G2
```

At review time, the Binance `*.binance.com` certificate path used `GeoTrust TLS RSA CA G1`, which DigiCert documents as being issued by `DigiCert Global Root G2`. DigiCert lists the root as valid until 2038-01-15.

This trust anchor is an implementation baseline, not an assumption that Binance will use the same certificate hierarchy forever. Re-check the live certificate chain before release hardening and whenever TLS validation begins failing.

See `docs/PHASE5_MARKET_DATA.md` for the market-data endpoint and symbol contract.

## Phase 6 weather TLS policy

Phase 6 weather data is read-only public forecast data from Open-Meteo.

Production host:

```text
https://api.open-meteo.com
```

The weather path does not require an API key for the project's current non-commercial usage. Exact home coordinates must remain local in `config.h` rather than being committed to the public repository.

The production TLS path follows the same baseline rules as market data:

1. Wi-Fi must be connected before a weather request is attempted.
2. The system clock must be synchronized before certificate-validating HTTPS.
3. HTTPS must use `NetworkClientSecure` with an explicitly configured trusted CA certificate.
4. Production code must never call `setInsecure()`.
5. Leaf-certificate or fingerprint pinning is not the default trust model.
6. TLS validation failure is a weather-data fetch failure; there is no insecure fallback.
7. A failed request must preserve the last valid weather snapshot rather than manufacture replacement weather data.

Initial Phase 6 trust anchor, reviewed 2026-10-04:

```text
ISRG Root X1
```

Let's Encrypt documents ISRG Root X1 as an active RSA root. Public certificate observations reviewed for `api.open-meteo.com` show Let's Encrypt-issued server chains. The root certificate has SHA-256 fingerprint:

```text
96:BC:EC:06:26:49:76:F3:74:60:77:9A:CF:28:C5:A7:
CF:E8:A3:C0:AA:E1:1A:8F:FC:EE:05:C0:BD:DF:08:C6
```

and certificate `notAfter` 2035-06-04.

The Open-Meteo leaf/intermediate chain can rotate. Therefore the Phase 6A-1 hardware test must verify the live chain through the ESP32-S3 TLS implementation before this trust path is considered hardware-verified.

See `docs/PHASE6_WEATHER.md` for the complete Phase 6A-0 provider/data/TLS review.

## Driver isolation

The E-paper driver must not depend on Wi-Fi, NTP/SNTP, HTTP, TLS, NVS, OTA, or cloud libraries.

The display stack should only handle:

- GPIO
- display bus I/O
- SSD1683 controller commands
- framebuffer operations
- refresh state

## Review rule

Any new dependency that can communicate externally or write persistent flash state should be explicitly documented before merge.
