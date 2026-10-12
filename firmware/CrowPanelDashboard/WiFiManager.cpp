#include "WiFiManager.h"

#include <WiFi.h>

WiFiManager::WiFiManager(uint32_t reconnectIntervalMs,
                         uint32_t connectTimeoutMs)
    : _ssid(nullptr),
      _password(nullptr),
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

  _ssid = ssid;
  _password = password != nullptr ? password : "";

  WiFi.mode(WIFI_STA);

  _started = true;
  _state = WiFiConnectionState::DISCONNECTED;
  _connectionAttempts = 0;

  startConnection(millis());
  return true;
}

void WiFiManager::startConnection(uint32_t now) {
  WiFi.begin(_ssid, _password);

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
