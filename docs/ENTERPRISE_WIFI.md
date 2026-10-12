# Enterprise Wi-Fi (PEAP) — pre-Phase-8 endurance gate

**Status (2026-10-12): code staged on `phase8a1-static-prototype`; Arduino compilation and ESP32-S3 on-device authentication have NOT yet been verified.**

## Scope and boundaries

- Adds **optional EAP-PEAP (802.1X)** connectivity to the existing `WiFiManager` state/retry interface; keeps Personal Wi-Fi as the default.
- The office Mac's EAPOL log confirmed **EAP-PEAP / EAP type 25**, but **PEAP inner method (for example, MSCHAPv2) is not independently established**.
- A Mac-keychain server certificate with EAP trust was found, and its signature verified against the retrieved public intermediate CA. That **does not prove** the live network necessarily presents that exact leaf or that the intermediate/root is an approved trust anchor for this SSID. No office-specific names or certificate contents are committed to the public repository.
- The Wi-Fi client *must* verify both a trusted EAP/RADIUS CA certificate and an expected server DNS name. Code fails closed when mandatory values are missing or security setup APIs fail. It does not fall back to CA-less PEAP.
- Only add a personal ESP32 device to the corporate network when company policy permits it. Beware office account secrets in firmware flash (can be extracted by someone with physical access); use a permitted device/service account if required.

## Local-only configuration

Keep `firmware/CrowPanelDashboard/config.h` ignored by Git, as before. Start from existing working `config.h` rather than overwriting weather coordinates or Phase 8 flags:

```cpp
#define WIFI_USE_ENTERPRISE 1
#define WIFI_SSID "YOUR_APPROVED_ENTERPRISE_SSID"
#define WIFI_EAP_IDENTITY "YOUR_OUTER_IDENTITY"    // commonly same as username
#define WIFI_EAP_USERNAME "YOUR_ACCOUNT"
#define WIFI_EAP_PASSWORD "YOUR_ACCOUNT_PASSWORD"
#define WIFI_EAP_SERVER_NAME "EXPECTED_RADIUS_CERT_DNS_NAME"

static const char WIFI_EAP_CA_PEM[] = R"EDPPEM(-----BEGIN CERTIFICATE-----
PASTE_VERIFIED_CA_BASE64_LINES_HERE
-----END CERTIFICATE-----
)EDPPEM";
```

For the user's MacBook-exported `.cer` file, if it is **DER** (as with the inspected file), convert **on the user's own Mac**:

```bash
openssl x509 -inform DER -in "TWCA Secure SSL Certification Authority.cer" \
  -outform PEM -out enterprise-ca.pem
openssl x509 -in enterprise-ca.pem -noout -subject -issuer -dates -fingerprint -sha256
```

Paste only the PEM block from `enterprise-ca.pem` into the C++ raw string. Verify its fingerprint against a **trusted independent reference** (e.g., the issuing CA's official documentation). Never paste a certificate's private key or a `.p12`/`.pfx` bundle. A public intermediate CA may or may not be accepted as the entire trust path by the ESP32 TLS/EAP implementation; troubleshoot verified trust chain on hardware rather than bypassing certificate validation.

The `WIFI_EAP_SERVER_NAME` value must represent the **actual approved RADIUS server certificate DNS name**, not merely a guessed domain. The discovered leaf is a candidate; it is not yet confirmed against the SSID by independent live evidence. A mismatch is intentionally an authentication failure.

The `WIFI_PASSWORD` Personal-mode setting remains unchanged/unused when Enterprise mode is enabled. Removing/commenting `WIFI_USE_ENTERPRISE` restores the existing Personal mode.

## Phase 8 full Dashboard test

Use **both** local flags as previously accepted for live Phase 8C-3:

```cpp
#define EDP_PHASE8B5B_APP_TEST 1
#define EDP_PHASE8C3_APP_TEST 1
```

Disable isolated preview/test flags. Continue to provide local `WEATHER_LATITUDE`/`WEATHER_LONGITUDE`. All existing boot paths invoke the same `beginConfiguredWiFi()` selector. Neither service polling nor the E-paper refresh sequence has changed.

## Expected checks (not yet accepted)

1. Arduino IDE 2.3.10 with Espressif board package **3.3.11**: verify successful compile and upload. If `CONFIG_ESP_WIFI_ENTERPRISE_SUPPORT` is disabled in the selected board build, code rejects Enterprise mode.
2. Secure setup must fail for an absent/malformed CA and incorrect server DNS name; **do not expose usernames/passwords in Serial**.
3. Confirm successful **PEAP 802.1X**, DHCP IP, RSSI icon, and NTP time sync; measure actual connection time (the existing manager has 15 s timeout, 10 s retry; enterprise EAP may require adjustment if logs show it is too short).
4. Confirm CA chain and server-name checks are enforced, and check that the actual PEAP inner authentication matches network requirements. If unsuccessful, inspect sanitized Serial logs, never retry with unvalidated PEAP.
5. Confirm Binance BTC/ETH/HYPE HTTPS and Open-Meteo HTTPS fetches all succeed using their existing *separate* HTTPS trust anchors; corporate proxy restrictions or TLS inspection can cause distinct failures.
6. Recover from Wi-Fi disconnection and verify no unintended E-paper refresh changes. Do not mark Phase 8 endurance PASS until the existing accepted full Dashboard operates with the chosen office network.

**Not changed:** SSD1683 driver, verified maintenance refresh behavior, weather/market/clock services, button/standby behavior, Dashboard typography/widgets, original certificate-validation policy for outgoing HTTPS.

## API references

- Arduino-ESP32 3.3.11 `WiFiSTA.h` / `STA.cpp`: `WiFi.begin(ssid, WPA2_AUTH_PEAP, identity, username, password, caPem)`.
- ESP-IDF 5.x `esp_eap_client_set_ca_cert`, `esp_eap_client_set_domain_name`, `esp_eap_client_set_eap_methods`: explicit trust anchor, server identity, and PEAP-method restrictions. Error returns are checked before attempting authentication.
- Before production, verify certificate time validation/clock bootstrap behavior on cold start without weakening the TLS trust or expected server identity.
