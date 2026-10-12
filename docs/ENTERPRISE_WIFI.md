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

## First on-panel PEAP connection test — troubleshooting (2026-10-12)

**Hardware observation:** The firmware compiled and uploaded to ESP32-S3. Serial showed `Wi-Fi connection attempt #1` and `#2` without a confirmed IP; on subsequent attempts, ESP-IDF logged `wifi:sta is connecting, cannot set config` and Arduino PEAP connection setup failed. This is a confirmed **reconfiguration/retry overlap**, not proof that RADIUS accepted or rejected credentials, or that the CA/server-name checks passed.

**Root cause in EDP:** `WiFiManager::startConnection()` repeated `WiFi.begin(... PEAP ...)` on every retry. Arduino-ESP32 3.3.11's `STAClass::connect(...)` calls `esp_wifi_set_config()`, which is rejected if the STA is still connecting. The Arduino STA library also has its own automatic reconnect enabled by default, potentially overlapping the application manager's retry loop.

**Pending compile + hardware verification** — conservative incremental fix submitted:
- For **Enterprise only**, disable Arduino's automatic STA reconnect and let the application manager control reconnect attempts. The Arduino core may still have an internal first-connect retry; do not interpret the limit below as a strict network-server attempt count.
- Perform CA, expected server DNS name, PEAP method, identity/password setup and `WiFi.begin()` **only on first attempt**. Subsequent connection attempts call `WiFi.reconnect()`, avoiding the `esp_wifi_set_config()` reconfiguration path.
- Allow at least **60 seconds** for PEAP setup instead of the Personal Wi-Fi 15-second window, followed by a 10-second retry delay. The verified Personal-mode defaults remain unchanged.
- Log **numeric and named Wi-Fi STA disconnection reasons** only (no SSID, usernames, passwords or enterprise hostname). Do not enable dumps that expose credentials.
- While the first office-network authentication diagnosis is pending, pause application-controlled PEAP attempts after **three consecutive unsuccessful connection attempts** to reduce risk of account lockout. Any IP-connected success resets the consecutive counter. After the pause, troubleshoot and **reset** to retry. Long-duration reliability behavior will be revisited after PEAP authentication passes.

**What to collect next:** Compile, upload, and capture the first 1–3 PEAP attempt state lines, the `PEAP STA disconnected: reason N (NAME)` events, any `PEAP connection timed out` messages, and a DHCP/NTP result if connected. Redact any infrastructure IP/hostname/SSID; never share the account credentials. If certificate validation, inner EAP, or server-name matching remains unsuccessful, troubleshoot those separately. Do not set `client.setInsecure()`, skip the EAP CA, or disable domain-name matching.

## Second hardware observation — reason 23 (2026-10-12)

The user compiled/uploaded the PEAP retry fix. The second live Serial trace shows `PEAP STA disconnected: reason 23 (802_1X_AUTH_FAILED)` twice during the first application-managed attempt. The previous `sta is connecting, cannot set config` error was **not observed in this trace**. **This does not identify which EAP stage failed or whether the office network accepted the credentials.** Two disconnect events do not necessarily mean two separate app-level PEAP attempts (the Arduino STA core may perform a first-connect retry).

Review of the current project `TimeService` confirms SNTP starts **only after Wi-Fi becomes connected**, while the EAP server TLS verification is performed **during Wi-Fi authentication**. On a cold boot with an unset RTC, this may interact with certificate validity time checks. Also keep investigating trusted CA chain, expected RADIUS server name, inner PEAP method, and office device policy rather than assuming the password is wrong.

A temporary **Serial-only** diagnostic was committed in `WiFiManager::beginEnterprisePeap()` to log:
- `PEAP pre-auth UTC: YYYY-MM-DD HH:MM:SS`
- `PEAP certificate time validity check: ENABLED/DISABLED` (as reported by `esp_eap_client_get_disable_time_check()`), or an error status.

This does **not** modify the clock, CA, expected server name, EAP authentication method, retries, or display refresh. Compile and confirm on hardware before relying on either diagnostic. Do not expose the actual username/password in log output, and avoid repeated resets after reason 23, to reduce account-lockout risk.

## Third hardware observation — time-check ruled out as immediate cause (2026-10-12)

The user compiled/uploaded the **Serial-only diagnostic** to the real CrowPanel ESP32-S3 and reported:

```text
PEAP pre-auth UTC: 1970-01-01 00:00:09
PEAP certificate time validity check: DISABLED
Wi-Fi connection attempt #1
Wi-Fi state: CONNECTING
PEAP STA disconnected: reason 23 (802_1X_AUTH_FAILED)
PEAP STA disconnected: reason 23 (802_1X_AUTH_FAILED)
```

**Verified on hardware:** the clock is unset prior to Wi-Fi, but the ESP-IDF EAP certificate validity-*date* check reports disabled. Thus a 1970 clock is **not a supported explanation** for this specific failure due to a strict validity-date check, and no clock bootstrap/time-check workaround should be added merely on this evidence. **Not verified:** RADIUS server certificate chain/name actually presented by the SSID, PEAP Phase 2 inner method, outer identity format, whether credentials are authorized for the device, and detailed TLS handshake failure stage. `reason 23` is generic and does not by itself prove a password mismatch or failure of the CA or expected server name checks. The two disconnect events may arise from driver-level retry within one application attempt.

**Release security gate:** the currently reported `DISABLED` certificate validity-date check is **not an acceptable finished security posture** for the project, even if CA-chain and RADIUS-name checks are configured. The production design must obtain a trusted pre-network time source (or another defensible validation strategy), then explicitly enable/verify expiration/not-before checks **before production acceptance**; do not disable trust-anchor or server-name validation to make PEAP work.

**Next safe diagnostic (no firmware change):** Compare the local ignored `WIFI_EAP_IDENTITY`/`WIFI_EAP_USERNAME` formatting with the Mac's accepted enterprise Wi-Fi login (do not publish the actual credential); verify the actual SSID's authorized RADIUS hostname and CA chain using IT/admin-provided configuration or a verified live certificate rather than only a Mac-keychain candidate. Confirm PEAP inner EAP method (e.g. MSCHAPv2) with the administrator. Do not continuously reset/retry against the office credential while the cause is unknown.

## macOS EAP trust-exception observation (2026-10-12)

Read-only macOS Sequoia `log show` output from an already working office connection repeatedly contained `eapolclient: trust exception applied, trust settings applied` during certificate trust evaluation, including at 08:44:36.525 (local time), immediately before the previously observed successful Mac `EAP-PEAP: successfully authenticated` at 08:44:36.911. Mac Keychain Access previously showed a **candidate** EAP server leaf certificate `jxwl.starlux-airlines.com` with customized trust settings (`Extensible Authentication (EAP): Always Trust`).

**Interpretation:** Mac EAP authentication is applying saved trust exceptions/settings; this makes a *local custom trust-policy difference* between the Mac and a strict ESP32 EAP client a plausible explanation of the ESP32's `reason 23 (802_1X_AUTH_FAILED)`. These logs **do not identify the certificate** that received the exception and do not identify whether the ESP32 rejected the server chain/hostname, failed an inner Phase 2 authentication step, or was denied by network access policy. Neither the Mac's success nor public CA signature verification is proof of the currently presented RADIUS identity.

**Safe next checks:** On the Mac, inspect local read-only `security dump-trust-settings` output for a certificate with the relevant EAP trust override (mask unrelated personal details). Ask the network administrator for the office SSID's expected RADIUS DNS name, root/intermediate CA chain and PEAP Phase 2 method, and whether a personally provisioned ESP32 device/account is permitted. Do not edit Keychain trust, remove EAP CA/domain validation, or repeatedly send credentials to the office network as a diagnostic workaround. The prior `PEAP certificate time validity check: DISABLED` remains a separate production-release security gap, not a proven cause of this reason 23.

## API references

- Arduino-ESP32 3.3.11 `WiFiSTA.h` / `STA.cpp`: `WiFi.begin(ssid, WPA2_AUTH_PEAP, identity, username, password, caPem)`.
- ESP-IDF 5.x `esp_eap_client_set_ca_cert`, `esp_eap_client_set_domain_name`, `esp_eap_client_set_eap_methods`: explicit trust anchor, server identity, and PEAP-method restrictions. Error returns are checked before attempting authentication.
- Before production, verify certificate time validation/clock bootstrap behavior on cold start without weakening the TLS trust or expected server identity.
