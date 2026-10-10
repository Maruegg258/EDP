# Phase 8A-2 — Weather Icon System

**Status (2026-10-11):** Initial 11-icon Gallery **ran and displayed normally on hardware**, but the user preferred the subsequently supplied thin-outline reference style. **Rev-B pixel assets have been traced from that user reference and committed**; source/header consistency PASS. The new Rev-B artwork still needs Arduino compile/upload and physical visual approval. Earlier notes below describe the first implementation.

## Purpose and boundaries

The user accepted the Rev-D2 792×272 static Dashboard before asking to settle weather icons **ahead of Phase 8B** live-data integration. The existing Rev-D2 top current-weather and six bottom forecast symbols still use the fixed `drawOutlineSun()` implementation. Do **not** silently replace those verified positions until the new art has been inspected.

All new icons are original project-owned, pure 1-bit 32×32 black/transparent pixels with no antialiasing, greyscale, dithering or runtime scaling. The drawing family uses restrained outlines; rain/snow/thunder retain visible differentiating strokes within the 32px grid. Each icon is **128 bytes** (32 rows × four MSB-first bytes), totaling **1,408 bytes of raw pixel data** before metadata and any compilation/storage optimizations.

Layer separation:

- `weather_icons_src/WeatherIcons32.icons`: hand-editable 32×32 ASCII bitmap matrices; `#` foreground, `.` transparent.
- `tools/generate_weather_icons.py`: deterministic Python 3 standard library generator/checker.
- `WeatherIconAssets.h`: generated `Bitmap1bpp` definitions.
- `WeatherIconSelector.h/.cpp`: provider-neutral `WeatherCondition` + `isDay` -> icon mapping; does **not** fetch data or refresh the panel.
- `WeatherIconGallery.h/.cpp`: pure one-frame 792×272 rendering test; does **not** own E-paper refresh or network activity.
- `CrowPanelDashboard.ino`: opt-in test only, using existing verified `maintenanceRefresh(frameBuffer)` once then controller sleep. All three experimental boot modes are mutually exclusive.

Production `WeatherService`, `WeatherWidgetMapper`, legacy `Icons.h`, `CrowEPD579`, market/clock widgets, physical navigation and `main` remain unchanged.

## Icon catalog and semantic mapping

| Asset | Gallery label | Source conditions |
|---|---|---|
| CLEAR_DAY | SUN DAY | CLEAR and MAINLY_CLEAR, `isDay=true` |
| CLEAR_NIGHT | MOON NIGHT | CLEAR and MAINLY_CLEAR, `isDay=false` |
| PARTLY_DAY | PARTLY DAY | PARTLY_CLOUDY, `isDay=true` |
| PARTLY_NIGHT | PARTLY NIGHT | PARTLY_CLOUDY, `isDay=false` |
| OVERCAST | CLOUD | OVERCAST |
| FOG | FOG | FOG |
| DRIZZLE | DRIZZLE | DRIZZLE and FREEZING_DRIZZLE |
| RAIN | RAIN | RAIN, FREEZING_RAIN and RAIN_SHOWERS |
| SNOW | SNOW | SNOW, SNOW_GRAINS and SNOW_SHOWERS |
| THUNDER | THUNDER | THUNDERSTORM and THUNDERSTORM_HAIL |
| UNKNOWN | UNKNOWN | UNKNOWN and unknown enum values |

This is a **visual grouping**, not loss of data precision: `WeatherService` continues retaining source WMO condition and code separately. Frozen precipitation and hail have intentionally been consolidated into the nearest legible main weather glyph for this small-panel initial version. Review against physical contrast before any production mapping decision.

Only current/hourly data have an `isDay` boolean in the existing WeatherService model. Future six-day daily icons will use daytime-style or non-day-dependent icons as **representative daily weather**, not fictitious night predictions. Six-day daily data expansion remains a separate Phase 8C task.

## How to test

1. Update your local checkout of `phase8a1-static-prototype` (keep `config.h` ignored).
2. In local `firmware/CrowPanelDashboard/config.h`, use:

   ```cpp
   // #define EDP_PHASE8_FONT_TEST 1
   // #define EDP_PHASE8A1_PREVIEW 1
   #define EDP_PHASE8_WEATHER_ICON_TEST 1
   ```

   When testing, disable the two other opt-in modes, including any earlier `#define` with value 1.
3. Arduino IDE: compile and upload `CrowPanelDashboard.ino` with the same ESP32-S3 board configuration used for Rev-D2.
4. At 115200 baud, observe `PASS: weather icon mapping self-check.` and `PASS: Phase 8A-2 gallery rendered once; Wi-Fi/APIs disabled.`
5. Photograph the gallery at usual viewing distance. Check sunny vs moon; sun/cloud vs moon/cloud; overcast vs fog; drizzle vs rain vs snow vs thunder; unknown; consistent optical size/line thickness; icon legibility at the x=396 controller seam; and absence of ghosting/blur after `maintenanceRefresh`.
6. Provide rejected icon names (if any), along with what looks too thin, crowded or ambiguous. Refine **only** those assets and regenerate the header; avoid changing the approved Rev-D2 Dashboard during this verification step.

No Serial commands or physical button actions are required; the page is rendered once on boot. To exit, comment out `EDP_PHASE8_WEATHER_ICON_TEST`, then upload another mode.

Optional source consistency check (not required for Arduino compile):

```bash
python3 tools/generate_weather_icons.py --check
```

## Verification checklist

- [x] Independent native candidate sprites authored: 11 × 32×32 / 1 bit.
- [x] All 11 source matrices match generated C++ MSB-first packed pixel data byte-for-byte.
- [x] Gallery contains all 11 icons and 17px captions inside 792×272.
- [x] Static selector covers all 16 existing condition categories, day/night branch tests, and unknown fallback.
- [x] Font test, Dashboard preview and new Gallery are mutually exclusive by preprocessor guard.
- [ ] ESP32-S3 Arduino IDE compile/upload.
- [ ] User visual approval of all candidate weather icon shapes and E-paper refresh appearance.
- [ ] Adopt accepted icons into the static Dashboard; only later integrate live WeatherService updates with application-owned dirty/refresh handling.

**Avoid interpreting source-level PASS as a completed hardware test.**

## Rev-B — Reference-style artwork revision (2026-10-11)

### Evidence and decision

The user confirmed the Phase 8A-2 initial eleven-icon Gallery displayed normally on the actual CrowPanel, but the **artwork style** was not the intended one. They provided a narrow black-background screenshot containing **eleven thin white-outline icons** in the desired order: sun, crescent moon, partly cloudy day, partly cloudy night, cloud, fog, drizzle, rain, snow, thunder, and question mark. The user explicitly requested replacing all eleven candidates with that reference family and **not** adding wind data.

### Implementation

- **Keep the user-supplied design's native optical proportions.** Pixel artwork was traced from the provided screenshot onto a 32×32 monochrome glyph canvas. Source strokes use a fixed brightness threshold (above 135 of 255) to turn its anti-aliased reference lines into crisp 1-bit pixels; source glyphs are centered and are **not** enlarged to fill the square. The result is a compact roughly 21–23px drawn footprint surrounded by transparent margins.
- White lines on the original black background become foreground black when composed by `GraphicsBW::drawBitmap` on the white E-paper. This is the same line geometry but inverted foreground color appropriate for the production black-and-white panel.
- All eleven `weather_icons_src/WeatherIcons32.icons` glyph matrices have been replaced, and `WeatherIconAssets.h` has been regenerated to match. The `tools/generate_weather_icons.py` input/output format remains unchanged.
- `WeatherIconGallery.cpp` and `WeatherIconSelector.cpp` require **no code edits**: they already reference the stable eleven names and 32×32 contract. The gallery remains a one-shot 4×3 page, rendering the original approved 17px label font.
- No changed weather classifications or WMO decoding, no separate wind icon, no daily-data expansion, and no edits to `WeatherService`, production `WeatherWidgetMapper`, `Phase8A1Preview.cpp`, the dual-SSD1683 driver, navigation, GPIO, TLS, Wi-Fi, or maintenance refresh.

### Verification and next test

- [x] 11×32 source matrices, each 128 bytes after MSB-first packing.
- [x] Independently regenerated `WeatherIconAssets.h` and compared every byte/pixel with the checked-in asset (PASS).
- [x] All visible glyph bounds remain within each 32×32 cell; existing Gallery coverage and selector still reference all eleven icon names (PASS).
- [ ] Arduino IDE ESP32-S3 compile/upload of the **Rev-B** assets.
- [ ] Physical CrowPanel inspection and acceptance of the thin reference-like strokes. Examine SUN DAY/MOON NIGHT/PARTLY DAY/PARTLY NIGHT, distinguish DRIZZLE/RAIN/SNOW/THUNDER, ensure UNKNOWN is readable and there is no blur/ghosting.
- [ ] After approval, adopt these glyphs in the static Dashboard (future 8A-2D); do **not** integrate unreviewed weather sprites into production.

### How to view Rev-B

Use the unchanged local `config.h` flags:

```cpp
// #define EDP_PHASE8_FONT_TEST 1
// #define EDP_PHASE8A1_PREVIEW 1
#define EDP_PHASE8_WEATHER_ICON_TEST 1
```

Pull latest `phase8a1-static-prototype`, Compile → Upload via Arduino IDE, then inspect the same 11-icon Gallery. No Python command is required for Arduino IDE users, and `main` remains untouched.
