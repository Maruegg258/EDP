#include "WiFiManager.h"

#include <WiFi.h>

WiFiManager::WiFiManager(uint32_t reconnectIntervalMs,
                         uint32_t connectTimeoutMs)
    : _authMode(AuthMode::PERSONAL),
      _ssid(nullptr),
      _password(nullptr),
      _identity(nullptr),
      _username(nullptr),
      _caPem(nullptr),
      _reconnectIntervalMs(reconnectIntervalMs),
      _connectTimeoutMs(connectTimeoutMs),
      _lastAttemptMs(0),
      _attemptStartedMs(0),
      _connectionAttempts(0),
      _state(WiFiConnectionState::DISCONNECTED),
      _started(false) {
}

bool WiFiManager::begin(const char* ssid, const char* password) {
  if (ssid == nullptr || ssid[0] == '\0') {
    return false;
  }

  _authMode = AuthMode::PERSONAL;
  _ssid = ssid;
  _password = password != nullptr ? password : "";

  WiFi.mode(WIFI_STA);

  _started = true;
  _state = WiFiConnectionState::DISCONNECTED;
  _connectionAttempts = 0;

  startConnection(millis());
  return true;
}

bool WiFiManager::beginEnterprise(const char* ssid, const char* identity,
                                  const char* username, const char* password,
                                  const char* caPem, bool useTtls) {
#if !CONFIG_ESP_WIFI_ENTERPRISE_SUPPORT
  Serial.println("Enterprise Wi-Fi unavailable in this ESP32 board build.");
  return false;
#else
  if (!ssid || !ssid[0] || !identity || !identity[0] ||
      !username || !username[0] || !password || !password[0] ||
      !caPem || !caPem[0]) {
    Serial.println("Enterprise Wi-Fi requires credentials and a trusted CA.");
    return false;
  }

  _authMode = useTtls ? AuthMode::ENTERPRISE_TTLS : AuthMode::ENTERPRISE_PEAP;
  _ssid = ssid;
  _identity = identity;
  _username = username;
  _password = password;
  _caPem = caPem;

  WiFi.mode(WIFI_STA);
  _started = true;
  _state = WiFiConnectionState::DISCONNECTED;
  _connectionAttempts = 0;
  startConnection(millis());
  return true;
#endif
}

void WiFiManager::startConnection(uint32_t now) {
#if CONFIG_ESP_WIFI_ENTERPRISE_SUPPORT
  if (_authMode != AuthMode::PERSONAL) {
    WiFi.begin(_ssid,
               _authMode == AuthMode::ENTERPRISE_TTLS ? WPA2_AUTH_TTLS : WPA2_AUTH_PEAP,
               _identity, _username, _password, _caPem);
  } else
#endif
  {
    WiFi.begin(_ssid, _password);
  }

  _lastAttemptMs = now;
  _attemptStartedMs = now;
  ++_connectionAttempts;
  _state = WiFiConnectionState::CONNECTING;
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
