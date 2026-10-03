#pragma once

#include <Arduino.h>

struct SecureHttpResponse {
  int statusCode;
  String body;
  String error;

  SecureHttpResponse()
      : statusCode(0) {
  }
};

class SecureHttpClient {
public:
  explicit SecureHttpClient(
      const char* rootCa,
      int32_t connectTimeoutMs = 10000,
      uint16_t readTimeoutMs = 10000);

  bool get(const char* url, SecureHttpResponse& response) const;

private:
  const char* _rootCa;
  int32_t _connectTimeoutMs;
  uint16_t _readTimeoutMs;
};
