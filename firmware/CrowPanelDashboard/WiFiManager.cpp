#include "WiFiManager.h"

#include <WiFi.h>
#include <cstring>
#if defined(CONFIG_ESP_WIFI_ENTERPRISE_SUPPORT) && CONFIG_ESP_WIFI_ENTERPRISE_SUPPORT
#include "esp_eap_client.h"
#include "esp_err.h"
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
                                      const char* caPem, const char* serverName) {
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
  // The Arduino STA core auto-reconnects by default using connect(),
  // which may rewrite station configuration while PEAP is still running.
  // This manager owns Enterprise retries; Personal mode is unchanged.
  WiFi.setAutoReconnect(false);
  WiFi.mode(WIFI_STA);
  WiFi.onEvent(
      [](arduino_event_id_t event, arduino_event_info_t info) {
        (void)event;
        const auto reason = info.wifi_sta_disconnected.reason;
        Serial.printf("PEAP STA disconnected: reason %u (%s)\\n",
                      static_cast<unsigned>(reason),
                      WiFi.STA.disconnectReasonName(
                          static_cast<wifi_err_reason_t>(reason)));
      },
      ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
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
    err = esp_eap_client_set_domain_name(_serverName);
    if (err != ESP_OK) {
      Serial.printf("FAIL: PEAP server name: %s\n", esp_err_to_name(err));
      _state = WiFiConnectionState::DISCONNECTED;
      return false;
    }
    err = esp_eap_client_set_eap_methods(ESP_EAP_TYPE_PEAP);
    if (err != ESP_OK) {
      Serial.printf("FAIL: PEAP-only method setup: %s\n", esp_err_to_name(err));
      _state = WiFiConnectionState::DISCONNECTED;
      return false;
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
    }
    return;
  }

  if (now - _lastAttemptMs >= _reconnectIntervalMs) {
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
