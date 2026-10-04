# Phase 6 — Weather

## Phase 6A-0 provider / data-contract / TLS review

**Status:** Design review complete (2026-10-04). Hardware validation begins in Phase 6A-1.

This checkpoint intentionally changes no display behavior. It defines the weather-provider boundary, minimum forecast payload, location/configuration policy, and TLS trust model before firmware starts calling a new external host.

## Provider decision

Initial provider:

```text
Open-Meteo Forecast API
https://api.open-meteo.com/v1/forecast
```

Reasons for the initial selection:

- HTTPS JSON API with no API key required for the free non-commercial service.
- The project is a personal dashboard use case and the expected polling rate is far below the provider's published free-service limits.
- One Forecast API request can provide current conditions, hourly forecast data, and daily forecast data.
- Weather conditions are represented by numeric WMO weather codes, so provider data can remain independent from project-owned icons and UI labels.
- The API supports an explicit timezone and constrained hourly/daily forecast ranges.

If the project later becomes a commercial product, the Open-Meteo licence / service plan must be reviewed again before release.

## Location policy

The Forecast API requires latitude and longitude.

The production firmware must not hard-code the user's real home coordinates into the public repository. The intended configuration boundary is local `config.h`, which is already excluded by `.gitignore`.

Planned local values:

```cpp
#define WEATHER_LATITUDE  ...
#define WEATHER_LONGITUDE ...
```

Only non-personal placeholders should be added to `config.example.h`.

Coordinates are not authentication secrets, but for a fixed home dashboard they are privacy-sensitive device configuration and should remain local.

## Forecast data contract

Phase 6 is being designed for more than a single current-temperature value. The service boundary must retain enough data for:

1. current weather
2. the next six future hourly forecast slots
3. today/tomorrow daily data so the UI can later render a tomorrow summary

Proposed request variables:

```text
current=
  temperature_2m,
  weather_code,
  is_day

hourly=
  temperature_2m,
  weather_code,
  precipitation_probability,
  is_day

daily=
  weather_code,
  temperature_2m_max,
  temperature_2m_min,
  precipitation_probability_max

forecast_hours=7
forecast_days=2
timezone=Asia/Taipei
```

The hourly range is deliberately seven rows because Open-Meteo defines `forecast_hours` relative to the current hour. The service can retain the current-hour row for alignment/validation and expose the following six rows as the six future hourly forecast slots.

The `current` object remains the authoritative current-condition value because it is provided separately from the hourly forecast.

Daily index 0 represents today and index 1 represents tomorrow when the API response is requested in the local timezone. Keeping both days also allows a later rule-based summary such as temperature rising/falling tomorrow.

Exact array alignment and response validation will be verified against a real payload in Phase 6A-2 before the data model is declared hardware-verified.

## Proposed service-owned model

The service should preserve provider semantics rather than immediately collapsing all conditions into three display icons.

Conceptual model:

```text
WeatherSnapshot
├── current
│   ├── temperatureC
│   ├── weatherCode
│   └── isDay
│
├── nextHours[6]
│   ├── time
│   ├── temperatureC
│   ├── weatherCode
│   ├── precipitationProbability
│   └── isDay
│
├── today
│   ├── weatherCode
│   ├── temperatureMaxC
│   ├── temperatureMinC
│   └── precipitationProbabilityMax
│
└── tomorrow
    ├── weatherCode
    ├── temperatureMaxC
    ├── temperatureMinC
    └── precipitationProbabilityMax
```

The provider's raw numeric WMO code should be preserved in the service snapshot.

A later application/UI mapping can convert those codes to the currently available project-owned icon families:

```text
SUN
CLOUD
RAIN
```

and Phase 8 may expand the icon vocabulary without changing the WeatherService provider contract.

## Weather summary policy

A short tomorrow summary does not require an AI/cloud text-generation service.

The application can derive deterministic facts from the retained today/tomorrow values, for example:

- tomorrow materially warmer / cooler than today
- rain likely tomorrow
- clear / cloudy / rainy representative condition

The exact text and thresholds are a UI/product decision and are not part of Phase 6A-0.

## Polling policy

Initial production target:

```text
Weather polling: 30 minutes
```

This remains inside the roadmap's 15–60 minute weather guideline.

Data polling and E-paper refresh remain separate. A successful weather request that produces content-identical visible state must not force a physical refresh.

## Failure policy

The WeatherService should follow the same high-level resilience pattern already used for market data:

```text
validated success
    -> replace last-valid weather snapshot

request / TLS / HTTP / parse / validation failure
    -> preserve the previous last-valid weather snapshot
    -> stage no fabricated replacement data
```

No zero-temperature, empty condition, or transport-error text should be injected into the Weather widget as if it were valid weather.

Long-duration staleness indication remains a Phase 9 reliability decision.

## TLS review

Production host:

```text
api.open-meteo.com
```

Public certificate observations reviewed on 2026-10-04 show the host using Let's Encrypt certificate chains. Let's Encrypt documents `ISRG Root X1` as an active RSA root CA.

Planned Phase 6 trust anchor:

```text
ISRG Root X1
SHA-256:
96:BC:EC:06:26:49:76:F3:74:60:77:9A:CF:28:C5:A7:
CF:E8:A3:C0:AA:E1:1A:8F:FC:EE:05:C0:BD:DF:08:C6
certificate notAfter: 2035-06-04
```

Trust strategy:

- add the public ISRG Root X1 certificate to `TlsTrustAnchors.h`
- create a weather-specific `SecureHttpClient` instance using that root
- do not replace the existing Binance/DigiCert client
- require Wi-Fi plus synchronized system time before HTTPS
- never call `setInsecure()`
- treat certificate validation failure as weather-fetch failure
- do not pin the short-lived Open-Meteo leaf certificate or a rotating Let's Encrypt intermediate

The live served chain can rotate independently of firmware. Therefore the root choice is a reviewed implementation baseline, not a claim that the upstream hierarchy can never change.

Most importantly, this TLS plan is **not yet hardware verified**. Phase 6A-1 must prove that the ESP32-S3 `NetworkClientSecure` path accepts the live `api.open-meteo.com` chain when anchored by ISRG Root X1.

## SecureHttpClient decision

The existing `SecureHttpClient` already accepts a CA certificate in its constructor and calls `NetworkClientSecure::setCACert()`.

Phase 6 therefore does not need to widen the HTTP abstraction or introduce insecure fallback logic. The intended application composition is:

```text
DigiCert Global Root G2
        |
        v
market SecureHttpClient
        |
        v
MarketDataService

ISRG Root X1
        |
        v
weather SecureHttpClient
        |
        v
WeatherService
```

This keeps trust anchors explicit per external service and avoids coupling WeatherService to the Binance trust configuration.

## Parsing decision

No parser implementation is added in Phase 6A-0.

The Phase 5 market payload is a small flat object and is handled by a narrow project-owned parser. The Phase 6 weather response contains nested objects plus aligned arrays, so the Phase 5 scalar parser should not simply be copied or stretched without review.

Phase 6A-2 will choose the smallest robust parsing approach after the real Phase 6A-1 payload has been observed. Any new parsing dependency must be explicit and documented.

## Phase 6A-0 conclusion

Approved implementation direction:

```text
Open-Meteo Forecast API
        |
        | certificate-validating HTTPS
        | ISRG Root X1
        v
weather SecureHttpClient
        |
        v
WeatherService
        |
        +-- current conditions
        +-- next six hourly forecasts
        +-- today + tomorrow daily forecasts
        |
        v
application mapping
        |
        v
existing WeatherWidgetState / DashboardUpdateCoalescer
```

No E-paper driver, refresh sequence, Dashboard coalescer, or existing market-data path needs to change for this checkpoint.

**Next checkpoint: Phase 6A-1 — add the weather trust anchor and a minimal Serial-only Open-Meteo HTTPS probe.**
