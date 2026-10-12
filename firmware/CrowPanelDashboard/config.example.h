#pragma once

// Copy this file to config.h and fill in local values.
// Never commit config.h.

#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

// Optional: switch to Enterprise Wi-Fi in your LOCAL config.h only.
// #define WIFI_USE_ENTERPRISE 1
// #define WIFI_EAP_IDENTITY "YOUR_OUTER_IDENTITY"
// #define WIFI_EAP_USERNAME "YOUR_USERNAME"
// #define WIFI_EAP_PASSWORD "YOUR_ENTERPRISE_PASSWORD"
// #define WIFI_EAP_TTLS 0  // 0=PEAP, 1=TTLS; confirm method with network admin
// #define WIFI_EAP_CA_PEM R"PEM(-----BEGIN CERTIFICATE-----
// ... trusted RADIUS CA certificate ...
// -----END CERTIFICATE-----
// )PEM"
// Never omit the trusted CA certificate. Do not put private credentials in Git.

// Phase 6 weather location.
// Keep your real coordinates only in local config.h.
// Uncomment and replace both values with decimal-degree numbers.
// #define WEATHER_LATITUDE  YOUR_LATITUDE_AS_DECIMAL
// #define WEATHER_LONGITUDE YOUR_LONGITUDE_AS_DECIMAL
