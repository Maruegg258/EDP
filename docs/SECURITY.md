# Security baseline

## Repository policy

This repository is public. Never commit:

- Wi-Fi passwords
- API keys or tokens
- private certificates or private keys
- device-specific secrets

Use `config.h` locally. It is excluded by `.gitignore`. Commit only `config.example.h`.

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
