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
- outbound HTTPS requests only to explicitly configured market-data endpoints
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
