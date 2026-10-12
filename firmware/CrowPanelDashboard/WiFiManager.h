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
  // PEAP with required CA + expected RADIUS server name.
  bool beginEnterprisePeap(const char* ssid, const char* identity,
                           const char* username, const char* password,
                           const char* caPem, const char* serverName);
  void tick();

  WiFiConnectionState state() const;
  bool isConnected() const;
  int32_t rssi() const;
  uint32_t connectionAttempts() const;

private:
  enum class AuthMode : uint8_t { PERSONAL, ENTERPRISE_PEAP };
  bool startConnection(uint32_t now);

  AuthMode _authMode;
  const char* _ssid;
  const char* _password;
  const char* _identity;
  const char* _username;
  const char* _caPem;
  const char* _serverName;

  uint32_t _reconnectIntervalMs;
  uint32_t _connectTimeoutMs;
  uint32_t _lastAttemptMs;
  uint32_t _attemptStartedMs;
  uint32_t _connectionAttempts;

  WiFiConnectionState _state;
  bool _started;
};
