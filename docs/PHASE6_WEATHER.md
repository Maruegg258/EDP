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

**Status:** PASS on real hardware (2026-10-05).

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

Hardware verification confirmed:

1. Firmware compiled and uploaded successfully.
2. The Phase 6A-2 WeatherService probe ran only after the existing Wi-Fi and synchronized-time prerequisites were satisfied.
3. The live Open-Meteo response parsed successfully through the dedicated `WeatherService`.
4. Current weather parsed correctly, including local timestamp, temperature, raw WMO weather code, and day/night state.
5. Exactly five future hourly slots were retained from `hourly[1..5]`, with timestamp, temperature, raw WMO code, precipitation probability, and day/night state.
6. The service crossed local midnight correctly: the current value belonged to the new local date, `today` matched that date, and `tomorrow` matched the following date.
7. Today/tomorrow daily minimum/maximum temperatures and maximum precipitation probabilities parsed successfully.
8. The deliberate invalid-coordinate request failed with `invalid weather coordinates` before network activity.
9. The previously valid `WeatherSnapshot` remained unchanged after that failed request, confirming last-valid preservation.
10. The service remained Serial-only in this checkpoint; no Weather widget state was staged.
11. Existing Phase 5 market-data and dashboard responsibilities remain separate from WeatherService.

Observed live-data examples also confirmed that the service must keep the raw provider semantics rather than infer UI meaning too early: an instantaneous clear condition can coexist with a more severe daily representative weather code and high daily precipitation probability. Semantic mapping remains Phase 6A-3.

Phase 6A-2 is therefore hardware-verified and complete.

## Phase 6A-3 implementation

**Status:** PASS on real hardware (2026-10-05).

Phase 6A-3 adds a provider-neutral semantic condition beside every retained raw WMO weather code. It remains Serial-only and does not map conditions to icons or stage Weather widget state.

### Normalized condition model

`WeatherCondition` is intentionally independent from `Icons.h`, `WeatherWidget`, Dashboard state, and E-paper code:

```text
UNKNOWN
CLEAR
MAINLY_CLEAR
PARTLY_CLOUDY
OVERCAST
FOG
DRIZZLE
FREEZING_DRIZZLE
RAIN
FREEZING_RAIN
SNOW
SNOW_GRAINS
RAIN_SHOWERS
SNOW_SHOWERS
THUNDERSTORM
THUNDERSTORM_HAIL
```

The raw numeric `weatherCode` remains stored beside `condition` in current, future-hour, today, and tomorrow values. This preserves provider detail such as intensity while allowing later application/UI code to consume stable project semantics.

```text
Open-Meteo WMO code
        |
        v
WeatherCondition mapper
        |
        +-- raw weatherCode retained
        |
        +-- normalized condition retained
        |
        v
WeatherSnapshot
```

### WMO mapping baseline

The mapping follows the Open-Meteo WMO interpretation table reviewed for Phase 6A-3:

```text
0           -> CLEAR
1           -> MAINLY_CLEAR
2           -> PARTLY_CLOUDY
3           -> OVERCAST
45, 48      -> FOG
51, 53, 55  -> DRIZZLE
56, 57      -> FREEZING_DRIZZLE
61, 63, 65  -> RAIN
66, 67      -> FREEZING_RAIN
71, 73, 75  -> SNOW
77          -> SNOW_GRAINS
80, 81, 82  -> RAIN_SHOWERS
85, 86      -> SNOW_SHOWERS
95, 97      -> THUNDERSTORM
96, 99      -> THUNDERSTORM_HAIL
other 0..99 -> UNKNOWN
```

Intensity remains available through the raw WMO code rather than multiplying the semantic enum into light/moderate/heavy variants. This keeps the application-facing model compact while allowing later UI refinement to distinguish intensity if desired.

`isDay` remains separate from `WeatherCondition`. For example, a future UI can render `CLEAR + isDay=false` as a moon icon without changing the service or WMO mapping.

### Application verification

The Phase 6A-3 application path adds a deterministic mapping self-check before examining the live weather snapshot.

The self-check verifies all 29 WMO codes documented in the reviewed Open-Meteo table:

```text
0, 1, 2, 3,
45, 48,
51, 53, 55,
56, 57,
61, 63, 65,
66, 67,
71, 73, 75,
77,
80, 81, 82,
85, 86,
95, 96, 97, 99
```

It also verifies that undefined codes such as `4` and `98` produce `UNKNOWN`.

The live WeatherService probe then prints both representations, for example:

```text
weather code: 0 -> CLEAR
weather code: 51 -> DRIZZLE
weather code: 95 -> THUNDERSTORM
```

The Phase 6A-2 invalid-coordinate last-valid regression is retained and now also compares the normalized `condition` fields.

### Architectural boundary

Phase 6A-3 deliberately does **not** decide:

- which bitmap corresponds to a condition
- whether CLEAR at night uses a moon asset
- which conditions may share an icon
- weather label wording
- dashboard layout for the six weather slots
- E-paper refresh timing

Those are application/UI concerns. Phase 6B will connect live weather semantics to the existing Weather widget, and richer assets/layout remain appropriate for Phase 8 refinement.

### Required hardware verification

1. firmware compiles and uploads
2. existing Phase 3 self-checks still pass
3. WMO mapping self-check reports PASS for all 29 documented codes plus UNKNOWN fallback
4. live Open-Meteo fetch/parse still succeeds
5. current weather prints both raw code and expected normalized condition
6. all five future-hour slots print both raw code and normalized condition
7. today/tomorrow print both raw code and normalized condition
8. induced invalid-coordinate failure still preserves the complete last-valid snapshot, including normalized conditions
9. Phase 5 BTC/ETH/HYPE behavior remains normal
10. no Weather widget state is staged
11. no E-paper refresh regression is observed

Phase 6A-3 remains unchecked in the roadmap until these results are confirmed on hardware.

Hardware verification confirmed:

1. Firmware compiled and uploaded successfully.
2. The deterministic mapping self-check passed all 29 documented WMO weather codes plus UNKNOWN fallback handling.
3. The live current condition preserved the raw provider code and produced the expected normalized semantic condition.
4. All five future hourly slots preserved raw codes and produced normalized conditions.
5. Today/tomorrow daily conditions preserved raw codes and produced normalized conditions independently from the instantaneous current condition.
6. The live sample exercised several semantic categories in one run, including `DRIZZLE`, `RAIN_SHOWERS`, `OVERCAST`, and `THUNDERSTORM`.
7. The service crossed local midnight within the five-hour forecast window without breaking condition mapping.
8. The deliberate invalid-coordinate request still failed as expected and the complete last-valid snapshot, including normalized condition fields, remained unchanged.
9. The Weather widget remained unchanged; no Weather dirty state was staged in this checkpoint.
10. Existing market-data and dashboard responsibilities remained independent of the new semantic mapping layer.

The hardware result confirms the intended separation:

```text
raw WMO provider code
        +
normalized WeatherCondition
        |
        v
WeatherSnapshot
        |
        v
application/UI mapping later
```

Phase 6A-3 is therefore hardware-verified and complete.

## Phase 6B-1 implementation

**Status:** PASS on real hardware (2026-10-05).

Phase 6B-1 is the first checkpoint that allows validated live weather to affect the visible dashboard. Only the **current** condition is presented. The five future-hour values and today/tomorrow values remain owned by `WeatherService` and are not yet laid out on the E-paper.

### UI adapter boundary

A dedicated `WeatherWidgetMapper` now bridges the service model to the existing Phase 3 Weather widget contract:

```text
WeatherService
    |
    +-- WeatherSnapshot.current
                |
                v
       WeatherWidgetMapper
                |
                +-- existing SUN / CLOUD / RAIN assets
                +-- short display label
                +-- one-decimal Celsius text
                |
                v
        WeatherWidgetState
                |
                v
DashboardUpdateCoalescer::stageWeather()
                |
                v
existing application flush
```

`WeatherService` still has no dependency on `Icons.h`, `WeatherWidgetState`, Dashboard state, the coalescer, graphics, or the E-paper driver.

### Current three-icon presentation policy

The current icon set predates the richer Phase 6 weather semantics, so Phase 6B-1 deliberately uses a conservative temporary mapping:

```text
CLEAR, MAINLY_CLEAR
    -> WEATHER_SUN

PARTLY_CLOUDY, OVERCAST, FOG, UNKNOWN
    -> WEATHER_CLOUD

DRIZZLE, FREEZING_DRIZZLE,
RAIN, FREEZING_RAIN,
SNOW, SNOW_GRAINS,
RAIN_SHOWERS, SNOW_SHOWERS,
THUNDERSTORM, THUNDERSTORM_HAIL
    -> WEATHER_RAIN
```

The label remains more descriptive than the temporary three-icon family, for example `DRIZZLE`, `SNOW`, `SHOWERS`, or `THUNDERSTORM`.

This is intentionally a UI mapping rather than a loss of service data. Raw WMO codes, normalized `WeatherCondition`, precipitation probabilities, and day/night flags remain available in `WeatherSnapshot`. Richer moon/fog/snow/thunder assets can therefore be added later without changing the provider/service contract.

### Startup behavior

The dashboard baseline no longer presents the old test fixture `SUN / 28 C` as though it were live weather.

Before the first validated weather result arrives, the Weather widget uses:

```text
icon: WEATHER_CLOUD
label: --
temperature: -- C
```

The non-null cloud bitmap is only a neutral placeholder required by the existing snapshot/widget contract. It is replaced only after a complete validated WeatherService fetch succeeds.

### Temperature presentation

The existing Weather widget state has a 16-byte temperature buffer. Phase 6B-1 formats current temperature with one decimal place:

```text
23.2 C
```

This preserves the current provider precision while remaining inside the existing widget layout/buffer contract.

### Coalescing behavior

The weather fetch occurs after the existing dashboard flush, alongside the application-owned external-data work. A successful weather result calls only:

```text
DashboardUpdateCoalescer::stageWeather(...)
```

It does **not** call an E-paper refresh.

The next application loop consumes the pending state through the already verified flush path. If market data is also pending, the dirty mask can contain Weather and Crypto bits together and still produce one physical refresh.

Phase 6B-1 intentionally remains a one-shot weather integration. The 30-minute production polling schedule, unchanged-value observation, and unavailable/recovery behavior belong to Phase 6B-2.

### Verification diagnostics

The Phase 6A-3 WMO semantic self-check remains temporarily active as a regression check. Phase 6B-1 also adds a deterministic `WeatherWidgetMapper` self-check covering every `WeatherCondition` value, ensuring:

- every condition maps to one of the current three non-null weather bitmaps
- every display label fits the existing 16-byte weather-label buffer
- temperature formatting fits the existing 16-byte temperature buffer

After a live fetch, Serial reports the mapped current Weather widget state and the pending dashboard dirty mask.

Expected success includes:

```text
PASS: WeatherCondition mapping self-check ...
PASS: WeatherWidgetMapper self-check ...

Weather widget mapping: <condition> -> <label> | <temperature>
Staged live weather; pending dirty: ... WEATHER ...

PASS candidate: live current weather staged through DashboardUpdateCoalescer.
WeatherService did not trigger E-paper refresh; existing application flush will consume the pending state.
```

### Required hardware verification

1. firmware compiles and uploads
2. existing Phase 3 self-checks still pass
3. WeatherCondition and WeatherWidgetMapper self-checks pass
4. startup Weather widget shows the placeholder rather than fake test weather
5. live Open-Meteo fetch/parse remains successful
6. current condition maps to the expected temporary SUN/CLOUD/RAIN family and short label
7. current temperature renders with one decimal place
8. `WEATHER` appears in the coalescer pending dirty mask after the first valid weather stage
9. the subsequent application flush visibly updates the Weather widget
10. WeatherService itself never invokes display refresh
11. BTC/ETH/HYPE, Clock, and Wi-Fi behavior remain normal
12. E-paper text/icon clarity remains normal after the weather update

Phase 6B-1 remains unchecked in the roadmap until these results are confirmed on hardware.

Hardware verification confirmed after the diagnostic correction:

1. Firmware compiled and uploaded successfully.
2. The WMO semantic mapping self-check passed all 29 documented codes plus UNKNOWN fallback.
3. The mapper-local WeatherWidget self-check passed for every `WeatherCondition`.
4. The live Open-Meteo response parsed successfully through `WeatherService`.
5. The live current condition was mapped into the existing Weather widget contract with the expected label and one-decimal Celsius temperature.
6. The Weather widget update was staged only through `DashboardUpdateCoalescer::stageWeather()`.
7. The observed pending dirty mask was `WEATHER | BTC | ETH | HYPE`, confirming weather and market updates can accumulate before one application-owned display flush.
8. The live Weather widget visibly updated on the panel and the user confirmed the hardware result was normal.
9. The WeatherService remained display-independent and did not call the E-paper driver.
10. The five future-hour slots and today/tomorrow daily values remained service-owned and were not yet added to the visible layout.
11. Existing BTC/ETH/HYPE, Clock, and Wi-Fi behavior remained normal.
12. No E-paper clarity or refresh regression was reported.

The first failed hardware attempt is retained above as a useful regression note: a cross-compilation-unit pointer-identity diagnostic falsely rejected identical header-level `static constexpr` bitmap assets. Moving that diagnostic into `WeatherWidgetMapper.cpp` corrected the test without changing production weather mapping behavior.

Phase 6B-1 is therefore hardware-verified and complete.

### Phase 6B-1 diagnostic correction

The first hardware attempt stopped before the live Open-Meteo fetch with:

```text
FAIL: WeatherWidget mapping for UNKNOWN
```

Root cause was the **diagnostic**, not the weather mapping itself.

`Icons.h` currently defines bitmap objects as header-level `static constexpr`. That gives each compilation unit its own object instance. The first diagnostic implementation built the actual mapped icon inside `WeatherWidgetMapper.cpp` but compared its pointer address against `&Icons::WEATHER_CLOUD` instantiated in `CrowPanelDashboard.ino`.

The bitmap contents and semantic mapping were the same, but the object addresses were not required to be identical across those compilation units. The application therefore falsely failed on the first `UNKNOWN -> WEATHER_CLOUD` test and returned before calling Open-Meteo or staging weather.

The correction keeps the icon pointer identity check inside `WeatherWidgetMapper.cpp`, in the same compilation unit that owns `iconForCondition()`. The application now only consumes the mapper self-check boolean and reports PASS/FAIL.

No WeatherService parsing, TLS behavior, live mapping policy, coalescer behavior, display driver, or E-paper refresh sequence was changed by this correction.


## Phase 6B-2 implementation

**Status:** Implementation committed; awaiting real-hardware verification.

Phase 6B-2 converts the Phase 6B-1 one-shot weather integration into the initial production polling flow and adds a temporary application-level diagnostic sequence for unchanged-value suppression, failure hold, and recovery.

### Production polling cadence

The application now owns:

```text
WEATHER_POLL_INTERVAL_MS = 30 minutes
```

The first eligible poll runs immediately after Wi-Fi is connected and system time is synchronized. Later production attempts use unsigned `millis()` subtraction:

```text
now - lastPoll >= interval
```

so normal 32-bit `millis()` wrap-around remains safe.

A deterministic cadence self-check verifies:

```text
first poll           -> due immediately
29:59.999            -> not due
30:00.000            -> due
millis() wrap-around -> still due correctly
```

The production interval remains 30 minutes during this diagnostic checkpoint; it is not shortened merely to make the hardware test finish faster.

### Poll attempt policy

Weather HTTPS work remains after the existing application display flush and remains gated by:

```text
Wi-Fi connected
+
TimeService synchronized
```

When a poll becomes due, the application records the attempt time before performing HTTPS. Therefore a failed API/TLS/parse attempt does not create a tight retry loop; the next ordinary production attempt remains on the 30-minute cadence.

A successful fetch is mapped through `WeatherWidgetMapper` and staged only through:

```text
DashboardUpdateCoalescer::stageWeather(...)
```

The application then examines only the `WEATHER` bit of the pending dirty mask:

```text
visible weather changed
    -> WEATHER dirty pending

visible weather unchanged
    -> no WEATHER dirty
```

Other simultaneously pending widgets do not affect this decision.

### Failure behavior

A production WeatherService failure stages no replacement value.

```text
weather fetch fails
        |
        +-- WeatherService retains last-valid WeatherSnapshot
        |
        +-- application stages no WeatherWidgetState
        |
        +-- displayed Weather widget remains last-valid
        |
        +-- no WEATHER dirty caused by the failure
```

The startup placeholder remains only when no valid weather has ever been obtained.

### Temporary Phase 6B-2 diagnostic

To verify the behavior without waiting through multiple 30-minute wall-clock intervals, Phase 6B-2 temporarily runs a diagnostic state machine after the first live Weather widget has been physically committed.

It performs these steps in order:

```text
1. Re-stage the exact displayed Weather widget
   -> WEATHER dirty must remain absent

2. Preserve the complete last-valid WeatherSnapshot

3. Inject deterministic WeatherService failure
   using invalid coordinates
   -> request fails before network activity
   -> complete last-valid snapshot must remain unchanged
   -> displayed Weather widget must remain unchanged
   -> no WEATHER dirty may be staged

4. Force one normal live recovery fetch
   -> success may be visibly unchanged or changed
   -> if changed, normal coalescer/application flush handles it

5. Wait until recovered Weather state is physically committed
   -> diagnostic PASS
```

The invalid-coordinate injection is only a deterministic way to exercise the already established WeatherService failure contract. Production polling never substitutes invalid coordinates.

The forced recovery fetch resets the ordinary 30-minute polling origin, so after the diagnostic finishes the next normal poll remains 30 minutes later.

This temporary diagnostic state machine is intended to be removed in Phase 6B-3 after hardware verification, just as the Phase 5B-2 market diagnostic was removed in Phase 5B-3.

### Expected Serial checkpoints

A successful run should include:

```text
PASS: WeatherCondition mapping self-check ...
PASS: WeatherWidgetMapper self-check ...
PASS: Weather 30-minute cadence self-check ...
Production weather polling interval: 30 minutes.

Phase 6B-2 production weather poll starting...
...
Weather staging pending dirty: ... [WEATHER|...]
Weather poll completed successfully ...

Phase 6B-2 weather unchanged/failure/recovery diagnostic starting...
First live Weather widget is physically committed.

Re-staged identical weather; pending dirty: ...
PASS: identical Weather widget created no WEATHER dirty.

Injecting deterministic WeatherService failure (invalid coordinates)...
Expected weather failure: invalid weather coordinates
PASS: Weather failure preserved last-valid service data and displayed Weather widget; no WEATHER dirty was staged.

Forcing one normal live Weather recovery fetch...
...

PASS: Phase 6B-2 weather diagnostics complete: unchanged suppression, failure hold, and live recovery verified.
Production weather cadence remains 30 minutes.
```

### Required hardware verification

1. firmware compiles and uploads
2. existing Phase 3 self-checks still pass
3. WMO and WeatherWidgetMapper regression checks still pass
4. 30-minute cadence self-check passes
5. the first production weather poll runs immediately after Wi-Fi + NTP readiness
6. live weather remains correctly displayed
7. re-staging identical visible Weather state produces no `WEATHER` dirty bit
8. the injected WeatherService failure preserves the complete last-valid snapshot
9. the failure does not replace or clear the displayed Weather widget
10. the failure does not create a `WEATHER` dirty bit or physical refresh
11. a normal recovery fetch succeeds
12. changed recovery data, if any, is staged through the coalescer and application flush only
13. BTC/ETH/HYPE, Clock, and Wi-Fi behavior remain normal
14. E-paper refresh quality remains normal

Phase 6B-2 remains unchecked in the roadmap until these results are confirmed on hardware.

### Phase 6B-2 compile correction

The first compile attempt failed with:

```text
'WeatherStageResult' does not name a type
```

The Phase 6B-2 implementation initially declared `WeatherStageResult` immediately before `fetchAndStageLiveWeather()` in the middle of the Arduino `.ino` file. Arduino's sketch preprocessing generates function prototypes ahead of later declarations, so the generated prototype referenced `WeatherStageResult` before that enum type was known.

The fix moves `WeatherStageResult` beside the existing top-level `UpdateResult` enum, before helper function definitions. No polling, WeatherService, UI staging, diagnostic, or E-paper behavior changed.

