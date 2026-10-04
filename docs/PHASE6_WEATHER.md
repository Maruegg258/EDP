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
2. a six-slot hourly strip: the current hour plus the next five future hourly forecast slots
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

forecast_hours=6
forecast_days=2
timezone=Asia/Taipei
```

The hourly range is deliberately six rows because Open-Meteo defines `forecast_hours` relative to the current hour. The service retains the first current-hour row for alignment with the `current` object and exposes the following five rows as future hourly slots. The intended UI strip is therefore six columns total: `NOW + 5 future hours`.

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
├── nextHours[5]
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
        +-- current-hour alignment + next five hourly forecasts
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

## Phase 6A-1 implementation

**Status:** PASS on real hardware (2026-10-04).

Phase 6A-1 adds only the transport probe needed to validate the new weather HTTPS path. It does not add `WeatherService`, JSON parsing, weather-widget staging, or any E-paper refresh behavior.

Firmware changes:

- adds public `ISRG_ROOT_X1` trust material to `TlsTrustAnchors.h`
- creates a weather-specific `SecureHttpClient`
- waits for Wi-Fi plus synchronized system time
- performs one Open-Meteo request through certificate-validating HTTPS
- prints HTTP status, payload size, and the raw response body to Serial
- leaves the Phase 5 BTC/ETH/HYPE production path active
- does not call `setInsecure()`
- does not stage Weather dirty state

The Phase 6A-1 request uses:

```text
forecast_hours=6
forecast_days=2
timezone=Asia/Taipei
```

The intended hourly response is six rows total:

```text
current-hour alignment + next five future hours
```

Real weather coordinates are intentionally not stored in the public repository. Before hardware verification, local `config.h` must contain numeric decimal-degree values:

```cpp
#define WEATHER_LATITUDE  ...
#define WEATHER_LONGITUDE ...
```

If either definition is absent, the firmware still compiles and continues the existing dashboard/market behavior, but the one-shot weather probe reports a Serial `SKIP` instead of sending a request.

Required hardware result before Phase 6A-1 can be checked complete:

1. normal Phase 3 self-checks still pass
2. Wi-Fi connects normally
3. NTP reaches `SYNCHRONIZED`
4. Phase 5 market polling remains normal
5. the weather probe receives HTTP 200 through `ISRG Root X1`
6. the raw JSON contains `current`, six aligned `hourly` rows, and two `daily` rows
7. no Weather widget value changes during this checkpoint
8. E-paper refresh quality remains unchanged

A successful HTTP request is necessary but the actual response shape will be examined before Phase 6A-2 chooses and implements the parser.

Hardware verification confirmed:

1. The firmware compiled and uploaded successfully with local weather coordinates supplied through uncommitted `config.h`.
2. Wi-Fi and NTP synchronization completed before the weather request.
3. `api.open-meteo.com` returned HTTP 200 through the weather-specific `SecureHttpClient` anchored by `ISRG Root X1`.
4. The response used `Asia/Taipei` with `utc_offset_seconds=28800`.
5. The `current` object contained time, interval, `temperature_2m`, `weather_code`, and `is_day`.
6. The `hourly` object contained exactly six aligned rows for time, temperature, weather code, precipitation probability, and day/night state: current-hour alignment plus the next five future hours.
7. The `daily` object contained exactly two aligned rows for today and tomorrow with weather code, maximum temperature, minimum temperature, and maximum precipitation probability.
8. The Weather widget was not staged or changed by this checkpoint.
9. The existing dashboard / market path remained independent of the weather transport probe.
10. No insecure TLS fallback was used.

Observed payload details important for Phase 6A-2 parser design:

- The `current.time` sample was inside the current hour, while the first hourly row was the top of that hour. Therefore the parser must align the first hourly slot by hour semantics and must **not** require `current.time` to equal `hourly.time[0]` exactly.
- The current temperature and the first hourly temperature were close but not identical. The `current` object remains authoritative for the "NOW" value; the hourly arrays are forecast slots and should not be forced to equal current observations.
- The daily weather code differed from the instantaneous current weather code, which is valid because daily data summarizes the day rather than the current instant. Phase 6A-2 must validate each scope independently.
- The API response reported a nearby model/grid coordinate rather than echoing the requested coordinates exactly. The parser must not reject a valid response merely because returned latitude/longitude differ slightly from the configured request point.

Phase 6A-1 is therefore hardware-verified and complete.


## Phase 6A-2 implementation

**Status:** Implementation committed; awaiting real-hardware verification.

Phase 6A-2 replaces the Phase 6A-1 raw-body application probe with a dedicated provider-facing `WeatherService`. It remains Serial-only and does not stage any Weather widget state.

### Service boundary

```text
ISRG Root X1
      |
      v
weather SecureHttpClient
      |
      v
WeatherService
      |
      +-- validated last-valid WeatherSnapshot
      |
      v
Serial diagnostics only
```

`WeatherService` owns the Open-Meteo URL contract and a durable last-valid snapshot. The application supplies only latitude/longitude and consumes validated values. No Dashboard, widget, graphics, or E-paper type is referenced by the service.

### Data model

The committed snapshot contains:

```text
WeatherSnapshot
├── current
│   ├── time
│   ├── temperatureC
│   ├── weatherCode
│   └── isDay
│
├── futureHours[5]
│   ├── time
│   ├── temperatureC
│   ├── weatherCode
│   ├── precipitationProbability
│   └── isDay
│
├── today
│   ├── date
│   ├── weatherCode
│   ├── temperatureMaxC
│   ├── temperatureMinC
│   └── precipitationProbabilityMax
│
└── tomorrow
    ├── date
    ├── weatherCode
    ├── temperatureMaxC
    ├── temperatureMinC
    └── precipitationProbabilityMax
```

The six-column display contract is intentionally represented as:

```text
NOW = current object
+1h .. +5h = hourly[1] .. hourly[5]
```

The first hourly row, `hourly[0]`, is consumed only as current-hour alignment/validation data. Current temperature, condition, and day/night state remain authoritative from the separate `current` object.

### Parser decision

No external JSON dependency is added in this checkpoint.

The service uses a project-owned narrow parser for the fixed Open-Meteo response contract. It is deliberately not a general-purpose JSON parser. The parser:

- finds the named `current`, `hourly`, and `daily` objects
- accepts the required scalar values only with valid JSON value termination
- requires exactly 6 values in every required hourly array
- requires exactly 2 values in every required daily array
- rejects missing, extra, malformed, non-finite, or out-of-range required values
- validates local ISO-style date/time strings
- requires `current` and `hourly[0]` to align to the same local hour, not the exact same minute
- requires `current` and `daily[0]` to align to the same local date
- does not require current and hourly temperatures/conditions to be identical
- does not compare returned API grid coordinates with the configured request coordinates
- preserves raw numeric WMO codes; semantic WMO interpretation remains Phase 6A-3

Initial numeric validation boundaries are intentionally provider-contract checks rather than UI policy:

```text
temperature: finite, -100 C .. +100 C
weather_code: integer 0 .. 99
precipitation probability: integer 0 .. 100
is_day: 0 or 1
daily max temperature >= daily min temperature
```

### Last-valid behavior

Parsing is performed into a temporary `WeatherSnapshot`. The stored snapshot is replaced only after the complete current/hourly/daily response passes validation.

Therefore:

```text
successful validated fetch
    -> commit new last-valid snapshot

invalid coordinates / TLS / HTTP / malformed payload / validation failure
    -> leave previous last-valid snapshot untouched
    -> expose the most recent request error
```

The Phase 6A-2 application probe performs one real weather fetch and then deliberately calls the service with invalid coordinates. The second call must fail before network activity and the previously valid snapshot must compare unchanged.

### Expected successful Serial shape

```text
Phase 6A-2 WeatherService probe starting...
Fetching and validating current + hourly + daily weather...

Parsed current weather:
  time: ...
  temperature: ... C
  weather code: ...
  is day: YES/NO

Parsed next five hourly forecast slots:
  +1h ...
  +2h ...
  +3h ...
  +4h ...
  +5h ...

Today ...
Tomorrow ...

Testing last-valid preservation with invalid coordinates...
Expected failure: invalid weather coordinates
PASS: WeatherService parsed the live forecast and preserved last-valid data after failure.
Hourly contract: NOW uses current; five future slots come from hourly[1..5].
Phase 6A-2 remains Serial-only; no Weather widget state is staged.
```

Required hardware verification:

1. firmware compiles and uploads
2. Phase 3 state/coalescing self-checks still pass
3. normal Wi-Fi/NTP/Clock/Wi-Fi UI behavior remains intact
4. Phase 5 BTC/ETH/HYPE market polling remains intact
5. the real Open-Meteo response parses successfully
6. current values print correctly
7. exactly five future hourly values print from the six-row API response
8. today and tomorrow daily values print correctly
9. the intentional invalid-coordinate request fails
10. the previous valid weather snapshot remains byte-for-value equivalent through that failed request
11. no Weather dirty bit is staged and the visible Weather widget is unchanged
12. no E-paper refresh-path regression is observed

Phase 6A-2 must remain unchecked in the roadmap until these results are confirmed on hardware.
