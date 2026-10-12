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
      _started(false) {
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
  WiFi.mode(WIFI_STA);
  _started = true;
  _state = WiFiConnectionState::DISCONNECTED;
  _connectionAttempts = 0;
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
    return;
  }

  if (_state == WiFiConnectionState::CONNECTED) {
    _state = WiFiConnectionState::DISCONNECTED;
    _lastAttemptMs = now;
    return;
  }

  if (_state == WiFiConnectionState::CONNECTING) {
    if (now - _attemptStartedMs >= _connectTimeoutMs) {
      WiFi.disconnect();
      _state = WiFiConnectionState::DISCONNECTED;
      _lastAttemptMs = now;
    }
    return;
  }

  if (now - _lastAttemptMs >= _reconnectIntervalMs) {
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
