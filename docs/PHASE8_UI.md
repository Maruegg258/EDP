# Phase 8A-1 — Static Dashboard visual prototype

**Status (2026-10-10):** Rev-C4 static preview with 34/17/14px JetBrains Mono was tested on the device and accepted as a visual baseline. Later layout tuning remains opt-in and unverified until re-tested; prior checkpoints below preserve historical context.

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


## Phase 8A-1 Rev-B / Step 1 — Font & Logo Assets (2026-10-10)

**Scope:** Asset-only revision on `phase8a1-static-prototype`. No positioning changes and no production integration. Local Arduino compile and revised hardware visual approval remain pending.

### Evidence and changes

- User's revised on-device photograph showed broken `10 OCT` / `SUNDAY` letters, irregular BTC/ETH/HYPE outlines, and coarse, visibly pixel-doubled large numbers.
- The former `Phase8A1HeaderFont.h` had missing/near-empty letter masks (notably C, S, D). The old glyph-generator lookup selected ambiguous source glyph name prefixes. Replaced with **37 explicitly addressed 17px glyphs** (A–Z, 0–9, space), to support future calendar strings beyond the fixed demonstration date. Inter Display Medium (SIL OFL 1.1) was rasterized as packed 1-bit bitmaps; no font file is committed.
- The old large numeric glyph metadata used widths of 14px with advances of 12px. Adjacent glyph logical cells overlapped. Replaced `Phase8A1Digits.h` with **native 34px tall** 1-bit digits/symbols; normal digits are 21px wide with 22px advances. Punctuation is compact and all glyph advances are at least their actual widths. Large text in the preview is now drawn at `scale=1` instead of doubling a 17px raster.
- Rebuilt `Phase8A1SmallFont.h` from the exact-name, already verified project-owned `Font5x7` assets into **7×11px** native masks; this removes the previous inconsistent spacing and protects against accidentally selecting prefix-matching glyph definitions.
- Replaced `Phase8A1Logos.h` with directly composed **30×30px** native black/white outline geometry for ETH, BTC, HYPE. There is no longer a resize of prior 48px assets. Small-logo readability, symmetry, and trademark recognizability require device review.
- Corrected the packing of the 6px header space glyph to one byte per row (17 bytes), consistent with the generic `BitmapFont` contract.

### Static source/readback checks

- Verified byte count `height × ceil(width/8)` for each raster, glyph non-empty masks except space, right-edge padding bit cleanliness, and `xAdvance >= width`.
- Re-read source to confirm the preview now draws large numerals at native scale.
- Checked current fixed example text widths (ETH 141px, BTC 141px, HYPE 119px, six-day summary 86px) against existing preview coordinates.
- The divider positions remain **y=81** / **y=181**, and the 6-day pitch remains 128px: Rev-B Step 2 will address layout, including moving the top divider to y=77.
- No changes to `CrowEPD579`, `GraphicsBW`, Network / Market / Weather services, navigation, or the opt-in preview boot logic.

**Verification caveat:** Static byte/metrics checks are not a substitute for compiling under Arduino ESP32-S3 and inspecting the resulting panel. The earlier prototype passed hardware testing; **Rev-B Step 1 has not yet been tested on hardware**.

### Step 1 acceptance checklist

- [x] Fix corrupt header masks and cover all basic calendar uppercase characters.
- [x] Convert large numeric rendering to non-overlapping native 34px glyphs.
- [x] Improve small numeric bitmap data and its spacing.
- [x] Redraw all three 30px crypto logos without bitmap rescaling.
- [x] Commit changes and read back critical font/logo data.
- [ ] Arduino IDE / ESP32-S3 compilation on local developer setup.
- [ ] Physical confirmation of repaired `10 OCT` / `SUNDAY`, cleaner price digits, ETH/BTC/HYPE outline quality.

## Phase 8A-1 Rev-B — Typography consistency pass (2026-10-10)

**Trigger:** The user supplied a new on-panel photograph (`IMG_5981.jpeg`). Their observation: characters and numbers became more attractive and the earlier date/weekday corruption disappeared, but different sections showed visibly inconsistent font sizes and stroke weights. This is visual feedback, not a complete Rev-B acceptance.

**Root cause confirmed by source:** Large time/price numerals already used an Inter-inspired 34px native bitmap, and 17px date/weekday letters used another raster of the same family; however, SUNNY, USDT PERP, and forecast weekdays were rendered using 5x7 bitmap glyphs at scale=2 (14px, doubled square pixels). Forecast numeric labels/temperature used a third 11px pixel-scaled font. Mixed families and resampling accounted for visible weight/shape mismatches.

**Changes (opt-in static preview only):**
- Establish intentional visual hierarchy: **34px** native time / prices, **17px** native date / day / uppercase English labels, **13px** native weather temperature / 6-day compact numbers.
- Use `Phase8A1HeaderFont::FONT` with `scale=1` for SUNNY, USDT PERP, MON–SAT, and the existing date/weekday labels. Remove `Font5x7` from this prototype renderer; core `Font5x7` file remains untouched.
- Rebuild `Phase8A1SmallFont.h` at 7×13 pixels: numeric/C glyphs down-rasterized from the existing 17px Inter-family bitmap by fractional area coverage; punctuation hand-designed at final pixel size; advance widths remain 8px (space 5px). Render at scale=1.
- Retain existing 34px numeric raster; it already shares the Inter visual family. Correct the remaining price-renderer zero-width guard to measure at scale=1, consistent with the actual draw scale.
- **No position changes.** Both divider lines are still at y=81 and y=181, and crypto/forecast item anchors remain fixed. y=77 and grid re-centering are reserved for Rev-B Step 2.

**Readback and validation:** All 16 small glyphs have correct row-major byte lengths and non-overlapping advances. Header (37 glyphs) and large numerals (16 glyphs) were independently rechecked. Calculated fixed demo values fit in existing text bounds: `SUNNY` 64px, `USDT PERP` 112px, forecast `25-29 / 10 %` 86px. New caption and day label heights fit their static zones. Data services, E-paper driver, maintenance sequence, and optional preview mode remain unchanged.

**Still required:** Compile/upload to hardware, inspect text shape and perceived stroke balance at normal viewing distance. The user has not yet accepted this new typography revision; do not mark it as hardware-verified.


## Phase 8A-1 Rev-C Font Reset — C1/C2 (2026-10-10)

Rev-B's attempted font polishing was stopped at the user's request because the result still lacked a consistent family appearance. The Rev-C design intentionally **starts from new, independently editable, native-size pixel sources** instead of patching the prior raster fonts.

- [x] Rev-C1 — define fixed-cell typography contract: `DashboardFont34` 22×34px numeric, `DashboardFont17` 12×17px uppercase/date, `DashboardFont13` 8×13px compact weather numeric.
- [x] Rev-C2 — author three new `fonts_src/*.glyphs` sources, generated `DashboardFont*.h` headers, and a deterministic Python 3 converter. Readback/bitmap metrics and independent re-encoding checks passed.
- [ ] Run local Python `tools/generate_dashboard_fonts.py --check`, Arduino compile and Rev-C3 **font-only test page on the actual panel**.
- [ ] Rev-C4 — replace fonts inside static Dashboard only after user visually accepts the independent font sample. No screen divider or data service changes before that.

Design/implementation notes: [PHASE8_FONT_REVC.md](PHASE8_FONT_REVC.md). This phase has **not** been declared hardware-verified. `Phase8A1Preview.cpp` still uses the prior font assets and keeps its original demo frame unchanged.


## Phase 8A-1 Rev-C3 — Font-only test pages (2026-10-10)

The three new Rev-C fonts are intentionally **not yet** used by `Phase8A1Preview.cpp`. An isolated `DashboardFontTest.cpp/.h` module now provides four pages (34px, 17px, 13px, side-by-side comparison). Compile with local `#define EDP_PHASE8_FONT_TEST 1` and make sure `EDP_PHASE8A1_PREVIEW` is disabled. At 115200 baud, send `1`/`2`/`3`/`4` in Arduino Serial Monitor to switch pages without reflashing; no Wi-Fi/HTTP/services or background refresh are active. The existing maintenance refresh is preserved. Source-level glyph coverage and pixel-bounds validation passed; **Arduino compile and device approval are pending**.

For complete instructions and sample strings see [PHASE8_FONT_REVC.md](PHASE8_FONT_REVC.md). Continue to Rev-C4 only after the user confirms acceptable weight/spacing on the physical panel.

## Phase 8A-1 Rev-C4 — Accepted JetBrains Mono applied to isolated static Dashboard (2026-10-10)

After real-panel evaluation, the user approved **34px (numeric)**, **17px (English labels)** and **14px (small weather numeric)** JetBrains Mono Medium. The former 13px size was judged too small. The opt-in `Phase8A1Preview.cpp` is now switched from Rev-B mixed font assets to `DashboardFont34.h`, `DashboardFont17.h` and `DashboardFont14.h`, all at scale 1. The old `DashboardFont13` remains in the font diagnostic only.

This is a **font substitution only**: no divider movement, weather/logo redrawing, Wi-Fi changes, real data, or maintenance-refresh changes. The existing 792×272 fixture positions and sample values are retained. Font coverage and static text widths fit the fixture's header, market and forecast bounding zones, but the full rendered Dashboard has **not yet been compiled or visually accepted** on the panel.

To view the new static Dashboard, use local `config.h` with `#define EDP_PHASE8A1_PREVIEW 1` and with `EDP_PHASE8_FONT_TEST` disabled. Build/upload, check Serial 115200 and take a panel photo to inspect the three sizes, labels, forecast 14px text width, x=396 seam, dividers, and refresh quality. The mock weather, prices and date are still fixed **not live**. See [PHASE8_FONT_REVC.md](PHASE8_FONT_REVC.md) for full criteria.

## Rev-C4 physical acceptance / layout handoff (2026-10-10)

The user confirms the **actual CrowPanel 5.79-inch display** running the Rev-C4 34/17/14px JetBrains Mono static Dashboard is visually **acceptable**. This is an on-panel visual approval of the unchanged Rev-C4 font-integrated mock, not a claim that all possible refresh/staleness/long-duration conditions have been tested. No additional photo or detailed Serial log was supplied at this handoff. The last Rev-C4 source is commit `c821ce3b6ea34078c9b10390242ff3d0102f0537`, retained in history for rollback.

**Next controlled step: Rev-D1 layout-only test.** Move only the top diamond-ended horizontal divider from y=81 to y=77; center icon/weekday group and 14px forecast numerical summary independently inside all six equal 128px cells. Preserve 34/17/14px font assets, all header content, three market groups, icons/logos, second divider (y=181), network/production path and `maintenanceRefresh()`. Rev-D2 market-group balancing is deferred until D1 is physically reviewed.

## Phase 8A-1 Rev-D1 — First layout-only alignment pass (2026-10-10)

**Status:** Source change committed, geometry bounds checked, **not yet compiled/uploaded or physically approved**. Baseline: user-approved Rev-C4 visual appearance. Keep the `phase8a1-static-prototype` feature branch and isolated `EDP_PHASE8A1_PREVIEW` mode.

Changes solely in `Phase8A1Preview.cpp`:

- Upper diamond-ended divider moves from `y=81` to **`y=77`**. The lower divider stays at `y=181`.
- Six forecast columns are each **128px**, spanning `x=12…780` in a centered grid with 12px outer margins on a 792px screen. Column origins are `12, 140, 268, 396, 524, 652`; the split at visible x=396 falls exactly between third/fourth columns.
- Each 32px outline-sun and 17px weekday label is centered **as a combined group**, retaining their 8px gap: the 76px group starts at `columnLeft+26`, and its weekday begins at `columnLeft+66`. Six-day `25-29 / 10 %` in the accepted 9×14px font measures 108px and begins at `columnLeft+10`, leaving symmetrical 10px internal margins.
- Content strings, letter heights, icon designs, header date/time/Wi-Fi coordinates, market logos/prices/labels and all vertical forecast anchors `y=200/209/243` are **unchanged**. No attempt to reflow crypto groups until separate Rev-D2.
- Code uses `GraphicsBW::textWidth()` to center elements and returns `false` on unsupported glyphs/overflow. Static bounds audit of all six columns, 14px summary and 17px labels passed. No changes to font data, `GraphicsBW`, `CrowEPD579`, network, navigation or maintenance refresh.

**Device test:** with local `EDP_PHASE8A1_PREVIEW=1` and `EDP_PHASE8_FONT_TEST` disabled, compile/upload the sketch and inspect the resulting full 792×272 static mock. Focus on upper divider clearance, uniform six-day group alignment and 13? **No**: the bottom numbers should still be approved **14px**, seam x=396, clip/ghosting behavior and consistency with the Rev-C4 reference. Request explicit user approval before Rev-D2.

The printed prices/weather are mock values; live data and production Dashboard are still not connected to this layout.
