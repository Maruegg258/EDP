#include "WiFiManager.h"

#include <WiFi.h>
#include <cstring>
#include <time.h>
#if defined(CONFIG_ESP_WIFI_ENTERPRISE_SUPPORT) && CONFIG_ESP_WIFI_ENTERPRISE_SUPPORT
#include "esp_eap_client.h"
#include "esp_err.h"
#include <mbedtls/x509_crt.h>
#endif

WiFiManager::WiFiManager(uint32_t reconnectIntervalMs,
                         uint32_t connectTimeoutMs)
    : _authMode(AuthMode::PERSONAL),
      _ssid(nullptr),
      _password(nullptr),
      _identity(nullptr),
      _username(nullptr),
      _caPem(nullptr),
      _serverName(nullptr),
      _reconnectIntervalMs(reconnectIntervalMs),
      _connectTimeoutMs(connectTimeoutMs),
      _lastAttemptMs(0),
      _attemptStartedMs(0),
      _connectionAttempts(0),
      _state(WiFiConnectionState::DISCONNECTED),
      _started(false),
      _peapDiagnostics(false),
      _peapAttemptsSinceSuccess(0) {
}

bool WiFiManager::begin(const char* ssid, const char* password) {
  if (!ssid || !ssid[0]) return false;
  _authMode = AuthMode::PERSONAL;
  _ssid = ssid;
  _password = password ? password : "";
  WiFi.mode(WIFI_STA);
  _started = true;
  _state = WiFiConnectionState::DISCONNECTED;
  _connectionAttempts = 0;
  return startConnection(millis());
}

bool WiFiManager::beginEnterprisePeap(const char* ssid, const char* identity,
                                      const char* username, const char* password,
                                      const char* caPem, const char* serverName,
                                      bool diagnostics) {
#if defined(CONFIG_ESP_WIFI_ENTERPRISE_SUPPORT) && CONFIG_ESP_WIFI_ENTERPRISE_SUPPORT
  // Match Arduino-ESP32 STA size constraints; fail before transmitting credentials.
  if (!ssid || !ssid[0] || strlen(ssid) > 32 ||
      !identity || !identity[0] || strlen(identity) > 64 ||
      !username || !username[0] || strlen(username) > 64 ||
      !password || !password[0] || strlen(password) > 64 ||
      !caPem || !strstr(caPem, "-----BEGIN CERTIFICATE-----") ||
      !strstr(caPem, "-----END CERTIFICATE-----") ||
      !serverName || !serverName[0] || strlen(serverName) > 255) {
    Serial.println("FAIL: invalid PEAP credentials, trusted CA, or server name.");
    return false;
  }
  _authMode = AuthMode::ENTERPRISE_PEAP;
  _ssid = ssid;
  _identity = identity;
  _username = username;
  _password = password;
  _caPem = caPem;
  _serverName = serverName;
  _peapDiagnostics = diagnostics;
  // Serial-only diagnostic: PEAP server TLS validation happens before
  // the dashboard can synchronize NTP. Do not disable certificate checks.
  const time_t preAuthEpoch = time(nullptr);
  struct tm preAuthUtc {};
  if (gmtime_r(&preAuthEpoch, &preAuthUtc) != nullptr) {
    Serial.printf("PEAP pre-auth UTC: %04d-%02d-%02d %02d:%02d:%02d\n",
                  preAuthUtc.tm_year + 1900, preAuthUtc.tm_mon + 1,
                  preAuthUtc.tm_mday, preAuthUtc.tm_hour,
                  preAuthUtc.tm_min, preAuthUtc.tm_sec);
  }
  bool certTimeCheckDisabled = false;
  const esp_err_t timeCheckStatus =
      esp_eap_client_get_disable_time_check(&certTimeCheckDisabled);
  if (timeCheckStatus == ESP_OK) {
    Serial.printf("PEAP certificate time validity check: %s\n",
                  certTimeCheckDisabled ? "DISABLED" : "ENABLED");
  } else {
    Serial.printf("PEAP certificate time check status unavailable: %s\n",
                  esp_err_to_name(timeCheckStatus));
  }
  if (_peapDiagnostics) {
    Serial.println("PEAP diag: one application attempt (Arduino driver may retry internally).");
    // Validate only the supplied CA PEM encoding, not the live server chain.
    mbedtls_x509_crt ca;
    mbedtls_x509_crt_init(&ca);
    const int parsed = mbedtls_x509_crt_parse(
        &ca, reinterpret_cast<const unsigned char*>(_caPem),
        strlen(_caPem) + 1);
    mbedtls_x509_crt_free(&ca);
    if (parsed != 0) {
      Serial.printf("PEAP diag: supplied CA PEM parse FAILED: %d\n", parsed);
      return false;
    }
    Serial.println("PEAP diag: supplied CA PEM parse OK (not server verified).");
  }

  // The Arduino STA core auto-reconnects by default using connect(),
  // which may rewrite station configuration while PEAP is still running.
  // This manager owns Enterprise retries; Personal mode is unchanged.
  WiFi.setAutoReconnect(false);
  WiFi.mode(WIFI_STA);
  WiFi.onEvent(
      [this](arduino_event_id_t event, arduino_event_info_t info) {
        (void)event;
        const auto reason = info.wifi_sta_disconnected.reason;
        Serial.printf("PEAP STA disconnected: reason %u (%s)\n",
                      static_cast<unsigned>(reason),
                      WiFi.STA.disconnectReasonName(
                          static_cast<wifi_err_reason_t>(reason)));
        if (_peapDiagnostics) {
          Serial.printf("PEAP diag: DISCONNECTED at +%lu ms; TLS vs Phase 2 UNKNOWN.\n",
                        static_cast<unsigned long>(millis() - _attemptStartedMs));
        }
      },
      ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
  if (_peapDiagnostics) {
    WiFi.onEvent(
        [this](arduino_event_id_t, arduino_event_info_t) {
          // 802.11 association is NOT evidence of a valid PEAP/TLS server.
          Serial.printf("PEAP diag: STA_ASSOCIATED at +%lu ms (NOT PEAP success).\n",
                        static_cast<unsigned long>(millis() - _attemptStartedMs));
        },
        ARDUINO_EVENT_WIFI_STA_CONNECTED);
    WiFi.onEvent(
        [this](arduino_event_id_t, arduino_event_info_t) {
          Serial.printf("PEAP diag: GOT_IP at +%lu ms (link operational).\n",
                        static_cast<unsigned long>(millis() - _attemptStartedMs));
        },
        ARDUINO_EVENT_WIFI_STA_GOT_IP);
  }
  _started = true;
  _state = WiFiConnectionState::DISCONNECTED;
  _connectionAttempts = 0;
  _peapAttemptsSinceSuccess = 0;
  if (!startConnection(millis())) {
    _started = false;
    return false;
  }
  return true;
#else
  (void)ssid;
  (void)identity;
  (void)username;
  (void)password;
  (void)caPem;
  (void)serverName;
  Serial.println("FAIL: ESP32 build lacks Wi-Fi Enterprise support.");
  return false;
#endif
}

bool WiFiManager::startConnection(uint32_t now) {
  _lastAttemptMs = now;
  _attemptStartedMs = now;
  ++_connectionAttempts;
#if defined(CONFIG_ESP_WIFI_ENTERPRISE_SUPPORT) && CONFIG_ESP_WIFI_ENTERPRISE_SUPPORT
  if (_authMode == AuthMode::ENTERPRISE_PEAP) {
    ++_peapAttemptsSinceSuccess;
    if (_connectionAttempts > 1) {
      // Do not repeat WiFi.begin(): it calls esp_wifi_set_config, which
      // fails with "sta is connecting, cannot set config" during PEAP.
      // reconnect() only issues esp_wifi_connect() with existing EAP data.
      if (!WiFi.reconnect()) {
        Serial.println("PEAP STA reconnect deferred; Wi-Fi driver is not ready.");
        _state = WiFiConnectionState::DISCONNECTED;
        return false;
      }
      _state = WiFiConnectionState::CONNECTING;
      return true;
    }

    // Run full CA/domain/method setup only on the first connection attempt.
    // Preflight EAP security settings: Arduino WiFi.begin does not check
    // every underlying ESP-IDF setup result. Fail closed on setup failure.
    esp_err_t err = esp_eap_client_set_ca_cert(
        reinterpret_cast<const unsigned char*>(_caPem), strlen(_caPem));
    if (err != ESP_OK) {
      Serial.printf("FAIL: PEAP CA setup: %s\n", esp_err_to_name(err));
      _state = WiFiConnectionState::DISCONNECTED;
      return false;
    }
    if (_peapDiagnostics) {
      Serial.println("PEAP diag: CA setup API OK (NOT live chain validation).");
    }
    err = esp_eap_client_set_domain_name(_serverName);
    if (err != ESP_OK) {
      Serial.printf("FAIL: PEAP server name: %s\n", esp_err_to_name(err));
      _state = WiFiConnectionState::DISCONNECTED;
      return false;
    }
    if (_peapDiagnostics) {
      Serial.println("PEAP diag: expected DNS-name setup OK (NOT a peer match).");
    }
    err = esp_eap_client_set_eap_methods(ESP_EAP_TYPE_PEAP);
    if (err != ESP_OK) {
      Serial.printf("FAIL: PEAP-only method setup: %s\n", esp_err_to_name(err));
      _state = WiFiConnectionState::DISCONNECTED;
      return false;
    }
    if (_peapDiagnostics) {
      Serial.println("PEAP diag: PEAP method setup OK; initiating association.");
    }
    if (WiFi.begin(_ssid, WPA2_AUTH_PEAP, _identity, _username,
                   _password, _caPem) == WL_CONNECT_FAILED) {
      Serial.println("FAIL: Arduino PEAP connection setup.");
      _state = WiFiConnectionState::DISCONNECTED;
      return false;
    }
  } else
#endif
  {
    WiFi.begin(_ssid, _password);
  }
  _state = WiFiConnectionState::CONNECTING;
  return true;
}

void WiFiManager::tick() {
  if (!_started) {
    return;
  }

  const uint32_t now = millis();
  const wl_status_t wifiStatus = WiFi.status();

  if (wifiStatus == WL_CONNECTED) {
    _state = WiFiConnectionState::CONNECTED;
    _peapAttemptsSinceSuccess = 0;
    return;
  }

  if (_state == WiFiConnectionState::CONNECTED) {
    _state = WiFiConnectionState::DISCONNECTED;
    _lastAttemptMs = now;
    return;
  }

  if (_state == WiFiConnectionState::CONNECTING) {
    // 802.1X can take longer than the Personal-mode 15-second timeout.
    const uint32_t timeoutMs =
        (_authMode == AuthMode::ENTERPRISE_PEAP && _connectTimeoutMs < 60000UL)
            ? 60000UL : _connectTimeoutMs;
    if (now - _attemptStartedMs >= timeoutMs) {
      Serial.println(_authMode == AuthMode::ENTERPRISE_PEAP
                         ? "PEAP connection timed out; disconnecting before retry."
                         : "Wi-Fi connection timed out.");
      WiFi.disconnect();
      _state = WiFiConnectionState::DISCONNECTED;
      _lastAttemptMs = now;
      if (_peapDiagnostics) {
        _started = false;
        Serial.println("PEAP diag: one-shot observation ended; no application retry.");
      }
    }
    return;
  }

  if (now - _lastAttemptMs >= _reconnectIntervalMs) {
    if (_peapDiagnostics && _authMode == AuthMode::ENTERPRISE_PEAP) {
      _started = false;
      Serial.println("PEAP diag: one-shot observation ended; no application retry.");
      return;
    }
    // Diagnostic safeguard for office accounts: after three consecutive
    // unsuccessful PEAP connection attempts, wait for a manual reset.
    // Resets on every successful IP connection; Personal mode unaffected.
    if (_authMode == AuthMode::ENTERPRISE_PEAP &&
        _peapAttemptsSinceSuccess >= 3) {
      Serial.println("PEAP paused after 3 failed attempts. Check EAP logs / CA "
                     "and server name; reset device to retry.");
      _started = false;
      return;
    }
    startConnection(now);
  }
}

WiFiConnectionState WiFiManager::state() const {
  return _state;
}

bool WiFiManager::isConnected() const {
  return _state == WiFiConnectionState::CONNECTED;
}

int32_t WiFiManager::rssi() const {
  if (!isConnected()) {
    return 0;
  }

  return WiFi.RSSI();
}

uint32_t WiFiManager::connectionAttempts() const {
  return _connectionAttempts;
}
