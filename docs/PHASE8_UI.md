# Phase 8A-1 — Static Dashboard visual prototype

**Status:** Initial 8A-1 static layout functionally verified by the user on-panel; revised 2026-10-10 visual composition committed to the same feature branch, pending its own compile/upload and hardware confirmation.

## Source of design

User-supplied `EDP.pdf`, page 1, approximately 825.36 × 283.56 PDF points. This aspect ratio closely matches the project-owned **792 × 272** visible pixel coordinate system. The PDF is a visual reference, not a source of live prices/weather. No PDF or local credentials are committed.

Agreed priorities: >90% of time on DASHBOARD, ETH/BTC/HYPE order, two diamond-ended horizontal separators, modern numeric font, detailed outline crypto marks, and six-day summary along the bottom. The revised visual reference supersedes the original PDF regarding ornament sizes and placement. WEATHER and MARKETS pages are deliberately unchanged.

## Revised static composition (approved visual reference, 2026-10-10)

`Phase8A1Preview.cpp` still renders exactly **one static 792 × 272 frame**, with the following visual refinement of the user's most recent approved image:

- Two thin separating rules at **y=81** and **y=181**, each with small filled diamonds at the line endpoints
- Header: left weather icon is now **32×32 px**, same outline-sun design as the lower strip; compact `SUNNY` and `26.9 C` text. Center group is `10 OCT ◆ 12:59 ◆ SUNDAY`, vertically aligned on y≈42 with 17px date/weekday glyphs and 34px clock digits (exact 50% character height); header separators use the same filled diamond motif as line endpoints. Original Wi-Fi production bitmap remains at upper-right
- Crypto row: ETH / BTC / HYPE prices `2493.56 / 82750.1 / 84.154` and `USDT PERP`, with **no inter-asset sparkle separators**. Coin silhouettes are replaced by project-owned **30×30 monochrome outlined** symbols (62.5% of the original 48px bitmap dimension, near the requested 60%). BTC/ETH echo the supplied stroke-based logo reference; HYPE is an outline in the same style
- Six equally pitched forecast columns: a **32×32 outline weather icon** and weekday label on a single top row, with a compact single-line `25-29 / 10 %` below each. Sample values and icon conditions intentionally match the approved static mock; they are NOT live weather
- `Phase8A1Digits.h` retains the experimental large numeric raster for time and crypto prices; `Phase8A1HeaderFont.h` holds 17px header letters derived from project-owned Font5x7; `Phase8A1SmallFont.h` is a hand-drawn compact 11px numeric font for the six-column summaries and small current temperature
- English labels elsewhere continue to use `Font5x7`

The display is black/white with no grayscale promise. Numeric glyphs in `Phase8A1Digits.h` were rasterized from **Inter Display Medium** (SIL Open Font License 1.1); only packed project-local data are included, no original font files. Outline logo and weather-icon readability remain subject to revised hardware testing.

## Preview isolation and safety

- Main branch and default firmware behavior remain unchanged.
- In the feature branch, static preview is explicitly gated behind `#define EDP_PHASE8A1_PREVIEW 1` in your **local ignored `config.h`**.
- Preview mode runs **before normal Wi-Fi startup**, draws one frame and then loops with `delay(1000)` without polling, refreshing, or processing buttons. This is by design: it is a visual-only snapshot, not a functional dashboard.
- Preview renders directly through the existing GraphicsBW framebuffer and calls the already verified `CrowEPD579::maintenanceRefresh()` once. This **is not a raw Full Refresh** and does not introduce a new controller sequence.
- Production `Dashboard.cpp`, widgets, state snapshots, coalescer, services, page navigation, and the E-paper driver remain untouched.
- No new network service, HTTP endpoint, user coordinates, Wi-Fi credentials, or private configuration are committed.
- Existing E-paper refresh sequence remains fast clear → physical white → previous RAM white → current RAM new frame → partial refresh → RAM sync.

## How to test on the panel

1. Obtain **branch `phase8a1-static-prototype`**, preserving your existing local `config.h` (do not commit or share it). If using a fresh folder, copy `config.example.h` into `config.h`.
2. Add this line to your own `config.h`: `#define EDP_PHASE8A1_PREVIEW 1`.
3. Compile the sketch `firmware/CrowPanelDashboard/CrowPanelDashboard.ino` in the currently working Arduino ESP32-S3 environment. Confirm compilation succeeds before uploading.
4. Upload, open Serial Monitor at **115200**, and check `EDP Phase 8A-1: STATIC MOCK DATA PREVIEW` followed by `PASS: static dashboard preview shown once; Wi-Fi/APIs disabled.`.
5. Inspect the panel for numeric weight and readability, logo details, separation marks, six forecast cards, text clipping, and content crossing the x=396 dual-controller seam. Record any ghosting, blurred type or leftover fragments after the maintenance refresh.
6. Remove or comment the opt-in flag and recompile/reflash when ready to return to ordinary production Dashboard behavior.

**The fixed date and financial/weather values are visual test fixtures and NOT current live data.** There is no manual refresh in this prototype; resetting with the opt-in flag still set redraws the sample again.

## Acceptance and next steps

- [x] Branch isolated from `main`.
- [x] Original 8A-1 static prototype compiled/uploaded and visually tested by the user. User reported the panel rendered normally and closely matched the first simulated reference (2026-10-10); no explicit electrical measurements made.
- [x] Approved follow-up visual adjustments committed on this branch: reduced upper-left weather, 50%-height date/weekday, diamond header separators, near-60% outline logos, removed middle sparkles, six compact single-line forecasts.
- [x] Source readback + bitmap byte-length + text-width and horizontal clipping checks completed for the revised static assets.
- [ ] **Revised** Phase 8A-1 Arduino compile/upload and final 792×272 panel confirmation, especially header centering, logo stroke clarity, bottom text sizes, dual-controller seam and E-paper refresh artifacts.
- [ ] User acceptance of revised artwork before any production data integration.

After receiving the hardware results, refine typography and logo masks before production data integration. Six genuine daily forecasts require a later WeatherService contract update and must not be represented as already implemented by this mock.
