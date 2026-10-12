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
  bool beginEnterprise(const char* ssid, const char* identity, const char* username,
                       const char* password, const char* caPem,
                       bool useTtls = false);
  void tick();

  WiFiConnectionState state() const;
  bool isConnected() const;
  int32_t rssi() const;
  uint32_t connectionAttempts() const;

private:
  void startConnection(uint32_t now);

  enum class AuthMode : uint8_t { PERSONAL, ENTERPRISE_PEAP, ENTERPRISE_TTLS };
  AuthMode _authMode;
  const char* _ssid;
  const char* _password;
  const char* _identity;
  const char* _username;
  const char* _caPem;

  uint32_t _reconnectIntervalMs;
  uint32_t _connectTimeoutMs;
  uint32_t _lastAttemptMs;
  uint32_t _attemptStartedMs;
  uint32_t _connectionAttempts;

  WiFiConnectionState _state;
  bool _started;
};
