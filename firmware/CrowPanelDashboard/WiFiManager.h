#pragma once

#include <Arduino.h>

enum class WiFiConnectionState : uint8_t {
  DISCONNECTED,
  CONNECTING,
  CONNECTED
};

class WiFiManager {
public:
  WiFiManager(uint32_t reconnectIntervalMs = 10000,
              uint32_t connectTimeoutMs = 15000);

  bool begin(const char* ssid, const char* password);
  void tick();

  WiFiConnectionState state() const;
  bool isConnected() const;
  int32_t rssi() const;
  uint32_t connectionAttempts() const;

private:
  void startConnection(uint32_t now);

  const char* _ssid;
  const char* _password;

  uint32_t _reconnectIntervalMs;
  uint32_t _connectTimeoutMs;
  uint32_t _lastAttemptMs;
  uint32_t _attemptStartedMs;
  uint32_t _connectionAttempts;

  WiFiConnectionState _state;
  bool _started;
};
