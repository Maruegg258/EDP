#include "SecureHttpClient.h"

#include <HTTPClient.h>
#include <NetworkClientSecure.h>

SecureHttpClient::SecureHttpClient(
    const char* rootCa,
    int32_t connectTimeoutMs,
    uint16_t readTimeoutMs)
    : _rootCa(rootCa),
      _connectTimeoutMs(connectTimeoutMs),
      _readTimeoutMs(readTimeoutMs) {
}

bool SecureHttpClient::get(
    const char* url,
    SecureHttpResponse& response) const {
  response.statusCode = 0;
  response.body = "";
  response.error = "";

  if (_rootCa == nullptr || _rootCa[0] == '\0' ||
      url == nullptr || url[0] == '\0') {
    response.error = "invalid HTTPS client configuration";
    return false;
  }

  NetworkClientSecure tlsClient;
  tlsClient.setCACert(_rootCa);

  HTTPClient https;
  https.setConnectTimeout(_connectTimeoutMs);
  https.setTimeout(_readTimeoutMs);
  https.setReuse(false);

  if (!https.begin(tlsClient, url)) {
    response.error = "HTTPS begin failed";
    return false;
  }

  response.statusCode = https.GET();

  if (response.statusCode > 0) {
    response.body = https.getString();
  } else {
    response.error = HTTPClient::errorToString(response.statusCode);
  }

  https.end();

  if (response.statusCode < 200 || response.statusCode >= 300) {
    if (response.error.length() == 0) {
      response.error = "HTTP status ";
      response.error += response.statusCode;
    }

    return false;
  }

  return true;
}
