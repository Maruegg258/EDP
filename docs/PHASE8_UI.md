# Phase 8A-1 — Static Dashboard visual prototype

**Status:** Implemented on `phase8a1-static-prototype` branch. Awaiting Arduino compile/upload and on-panel visual validation.

## Source of design

User-supplied `EDP.pdf`, page 1, approximately 825.36 × 283.56 PDF points. This aspect ratio closely matches the project-owned **792 × 272** visible pixel coordinate system. The PDF is a visual reference, not a source of live prices/weather. No PDF or local credentials are committed.

Agreed priorities: >90% of time on DASHBOARD, preserve an ETH/BTC/HYPE order, two diamond-ended horizontal separators, small sparkle separators, modern numeric font, more detailed crypto marks, six-day summary along the bottom. WEATHER and MARKETS pages are deliberately unchanged.

## Prototype contents

`Phase8A1Preview.cpp` renders exactly **one static 792 × 272 frame** with:

- Y=67 and Y=181 dividing rules, black diamond endpoints
- Upper row: current weather at left, `10 OCT ✦ 12:59 ✦ SUNDAY` centered, existing production Wi-Fi strong icon at upper-right
- ETH / BTC / HYPE priced `2493.56 / 82750.1 / 84.154`, each with a 48×48 1-bit logo derived from the uploaded design sample, plus `USDT PERP`
- Six sample forecast slots MON through SAT with condition icons, integer min/max values (e.g., `25-29 C`) and daily precipitation percentages
- `Phase8A1Digits.h`: experimental numeric glyphs with tabular digit advances and three draw scales: time and prices use scale 2; temperatures and daily precipitation use scale 1. English labels continue to use `Font5x7`

The display is black/white. The original colorful brand-circle backgrounds were converted into solid dark monochrome silhouettes with white-negative marks; details, balance, line weights and dithering remain open to user testing and revision. Numeric bitmaps were rasterized from **Inter Display Medium** (SIL Open Font License 1.1), then committed as project-local packed 1-bit bitmap data only. The original font file is **not** included.

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
- [x] Static renderer + font assets + high-detail black/white logo assets + safe opt-in boot path committed.
- [ ] Local Arduino compile confirmed.
- [ ] Physical 792×272 rendering checked by user, including labels, price widths, weather strip legibility and SSD1683 seam.
- [ ] Hardware results recorded and marked accepted.

After receiving the hardware results, refine typography and logo masks before production data integration. Six genuine daily forecasts require a later WeatherService contract update and must not be represented as already implemented by this mock.
