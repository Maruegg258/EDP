# Phase 8A-1 Rev-C — Dashboard Font Reset

**Status (2026-10-10):** Rev-C3-JB2: user visually accepted JetBrains Mono 34px/17px and found 13px too small. A separate 9x14 candidate and five-page isolated test are committed, pending 14px compile/device evaluation. Earlier Rev-C1/C2/C3-JB1 notes below are retained as history.

## Rationale

Hardware photos from Rev-B showed inconsistent font appearance across the clock, crypto numbers, English captions, and 6-day forecast. Rev-C intentionally stops patching the `Phase8A1Digits`, `Phase8A1HeaderFont` and `Phase8A1SmallFont` raster assets. Build an entirely new, project-owned, explicitly editable, fixed-cell font family instead.

**Source of truth for font design:** `firmware/CrowPanelDashboard/fonts_src/DashboardFont*.glyphs`. These files contain literal `#` (black) and `.` (white) rows at the final 1-bit pixel resolution. The generated `DashboardFont*.h` files are outputs, not editable design sources.

## Rev-C1: Fixed native font metrics

| Font | Cell width/advance | Cell height/lineHeight | Use | Coverage | Status |
| --- | ---: | ---: | --- | --- | --- |
| DashboardFont34 | 22 px | 34 px | Clock, BTC/ETH/HYPE prices | 0–9, `.`, `:`, `-`, `+`, space (15 glyphs) | New source complete |
| DashboardFont17 | 12 px | 17 px | Date, weekday, SUNNY, USDT PERP, forecast weekday labels | A–Z, 0–9, space, `-`, `/`, `.`, `:`, `+`, `%` (43 glyphs) | New source complete |
| DashboardFont13 | 8 px | 13 px | Current weather temperature, 6-day temperature/rain | 0–9, `.`, `-`, `/`, `%`, `+`, `C`, space (17 glyphs) | New source complete |

**Precisely defined:** equal *cell dimensions and advance* per font. Ink shapes need not be identical rectangles: `1` legitimately has less black area than `8`, and `.` does not occupy the full cell height. A consistent pixel grid, baseline rules, line weight and character spacing are the objectives. All draws will use `scale=1`.

Every size is authored independently in its native cell. No font bitmap is derived by nearest-neighbor doubling/shrinking another size. Native strokes use shared geometric/sans-serif design principles, with pixel-level corrections for small sizes. The user should assess visual weight on the actual panel in Rev-C3, before these are treated as final production fonts.

## Rev-C2: New files

- `firmware/CrowPanelDashboard/fonts_src/DashboardFont34.glyphs`
- `firmware/CrowPanelDashboard/fonts_src/DashboardFont17.glyphs`
- `firmware/CrowPanelDashboard/fonts_src/DashboardFont13.glyphs`
- `firmware/CrowPanelDashboard/DashboardFont34.h`
- `firmware/CrowPanelDashboard/DashboardFont17.h`
- `firmware/CrowPanelDashboard/DashboardFont13.h`
- `tools/generate_dashboard_fonts.py`

### Source format

The source begins with `FONT DashboardFont34 22 34` (adjust for 17/13), then includes repeated blocks:

```text
GLYPH U+0030
......................
....(22 pixel characters per row)....
END
```

The demonstration above is intentionally shortened. Real `.glyphs` blocks have **exactly** the declared number of rows, all with the required number of pixel characters. Each glyph is addressed unambiguously using its `U+XXXX` code point, avoiding the earlier font-generator prefix collision.

### Regeneration

Run from the repository root:

```bash
python3 tools/generate_dashboard_fonts.py
python3 tools/generate_dashboard_fonts.py --check
```

Or generate/check one font:

```bash
python3 tools/generate_dashboard_fonts.py --font 17
python3 tools/generate_dashboard_fonts.py --font 17 --check
```

The generator uses Python standard library only, enforces the exact required glyph set, checks dimensions and nonempty non-space glyphs, and packs row-major MSB-first pixels into the existing `BitmapFont` API. `--check` fails if any checked-in output differs from the native source.

**Maintenance rule:** edit the readable `.glyphs`, regenerate all three headers, review diffs, run `--check`, then compile and physically test. Do not manually edit generated hex bytes.

## Static verification

- [x] 34px: 15 glyphs, fixed 22 × 34; readback byte length `34 × ceil(22/8)` per glyph.
- [x] 17px: 43 glyphs, fixed 12 × 17; readback byte length `17 × ceil(12/8)` per glyph.
- [x] 13px: 17 glyphs, fixed 8 × 13; readback byte length `13 × ceil(8/8)` per glyph.
- [x] Correct required Unicode/ASCII code points, no duplicate or accidental blank glyph.
- [x] All `BitmapGlyph` metrics use identical `width`, `height` and `xAdvance` within each size.
- [x] Independent source-to-header re-encoding comparison: all three stored headers matched the checked-in native pixel matrices.
- [x] Per-font side-bearing review; problem glyphs with ink against right-most column were repaired in their sources.
- [ ] Run Python generator `--check` in normal local developer toolchain.
- [ ] Compile with Arduino ESP32-S3 toolchain.
- [ ] Rev-C3 visual test sheet and device confirmation.

## Rev-C3 — Font-only device test (implemented, hardware approval pending)

`DashboardFontTest.h/.cpp` provide a standalone 792×272 composition renderer. It uses **only** the three Rev-C font families. It does not know how to refresh the controller or contact the network.

Activate it by using the firmware from **branch `phase8a1-static-prototype`** and adding this one line to the **ignored local** `config.h`:

```cpp
#define EDP_PHASE8_FONT_TEST 1
```

**IMPORTANT:** If your local `config.h` still contains `#define EDP_PHASE8A1_PREVIEW 1`, comment out or remove that older switch first. A build-time error explicitly disallows activating both.

In Arduino IDE compile/upload the sketch normally, then open the USB Serial Monitor at **115200 baud**. At boot, test sheet 1 displays automatically. Send a single digit `1`, `2`, `3` or `4` to choose a test page. Line ending is optional (CR/LF are ignored). **Upload only once; serial commands change the pages.** No network, timed data updates or physical-button handling occurs while this mode is active. Only changing the page triggers one maintenance refresh; repeated same-page commands do not refresh.

| Page | Samples (all native 1× BitmapFont pixels) | Look for |
| --- | --- | --- |
| 1: 34px | `0123456789`, `12:59`, `2493.56`, `82750.1`, `84.154`, `+-.: 012345` | Consistent numeric width, vertical alignment and 0/1/8 weight balance |
| 2: 17px | `ABCDEFGHIJKLM`, `NOPQRSTUVWXYZ`, `0123456789`, `10 OCT SUNDAY`, `SUNNY USDT PERP`, `MON TUE WED THU FRI SAT` | No lost letters or badly weighted uppercase / numbers |
| 3: 13px | `0123456789`, `26.9 C  25-29 / 10 %`, `-12.5 C  25-29 / 100 %`, `+3.2 C  -30-40 / 95 %` | Period, slash, minus, percent legible at real size; long daily values fit |
| 4: comparison | 34px `12:59` and `82750.1`, 17px `12:59` / `MON TUE WED THU`, 13px `26.9 C` / `25-29 / 100 %` | Shared family impression and useful relative sizing |

The test intentionally draws several sample strings across the E-paper controller seam near visible x=396, helping identify seam-specific artifacts. The panel retains the last test image even after `display.sleep()`.

### Rev-C3 code verification

- [x] Static renderer and opt-in boot+serial switching committed on feature branch
- [x] No `Font5x7`, old `Phase8A1Digits` or Dashboard widget paths in the new renderer
- [x] 31 literal samples validated against required per-font glyph coverage and 792×272 bounds
- [x] Font source/header consistency retained; all samples fit the declared fixed cell widths
- [x] Only `maintenanceRefresh(frameBuffer)` used for E-paper hardware updates
- [x] Compile-time exclusive-choice guard between font test and old static Dashboard preview
- [ ] Execute `python3 tools/generate_dashboard_fonts.py --check` on the actual repo checkout
- [ ] Arduino IDE / ESP32-S3 compilation and upload
- [ ] On-panel inspection by user, photos of all 4 sheets, and explicit acceptance/adjustments

These are **static source-level checks only**. Arduino compile and hardware testing have **not** been carried out by the assistant.

### Expected serial output

```text
EDP Phase 8A-1 Rev-C3 FONT TEST: native 1-bit 34/17/13px.
Send 1, 2, 3, or 4 via Serial Monitor to choose a font sheet.
PASS: Rev-C3 font sheet 1/4 rendered; Wi-Fi/APIs disabled.
```

After entering `2`:

```text
Rev-C3 rendering font sheet 2/4...
PASS: font sheet 2/4 displayed.
```

To exit test mode, remove/comment the local `EDP_PHASE8_FONT_TEST` flag and reflash. The existing `EDP_PHASE8A1_PREVIEW` flag can then be selected again if desired.

## Rev-C4 — Deliberately deferred

**Rev-C4:** After user approval of the test sheets, replace experimental font references in `Phase8A1Preview.cpp`, recompute widths/positions and then resume the existing divider/layout milestones. Never infer approval from automated bitmap checks.

## Scope and safety

The three new font families are **used only in the opt-in diagnostic renderer**, not by the normal static Dashboard preview or by production services. Existing `Phase8A1Preview.cpp` and Rev-B fonts remain unchanged. The `main` branch, E-paper driver, maintenance refresh sequence and all network services are unchanged. No Wi-Fi secrets or external font files were added.

## Rev-C3-JB1 — JetBrains Mono font adoption (2026-10-10)

**Status: font assets and four-sheet test renderer prepared; Arduino compile, upload, and on-panel acceptance remain unverified.** This replaces only the Rev-C2 hand-drawn diagnostic font **assets** on the existing `phase8a1-static-prototype` branch. The tested 34/17/13px fixed-cell `BitmapFont` interface and the Rev-C3 four-sheet Serial page selector remain the same. No production Dashboard, normal static preview, E-paper driver, network service, or refresh sequence changes.

Font provenance: [JetBrains Mono](https://github.com/JetBrains/JetBrainsMono), **Medium** weight, official `fonts/ttf/JetBrainsMono-Medium.ttf` blob SHA `dc2e5d08677d603b6755dc6c34fc8ad9aa6fc8a5` on its `master` branch at import. Copyright 2020 The JetBrains Mono Project Authors; SIL Open Font License 1.1. The exact upstream license text is retained in `fonts_src/OFL-JetBrainsMono.txt`. **No TTF/OTF font files are committed.** The original upstream typeface is not the earlier generative comparison illustration.

The glyphs are rasterized from actual TrueType outlines (quadratic contours and counters), using 4×4 supersampling and a binary black/white coverage threshold. Stored outputs are **pure 1-bit** ASCII matrices in `fonts_src/DashboardFont*.glyphs` and row-major MSB-first `DashboardFont*.h`. Grid sizes/advances remain 22×34, 12×17, and 8×13 respectively; the underlying typeface is uniformly scaled and centered within each existing cell. Raster baseline/scale configuration: 34: baseline 30, scale 22/600; 17: baseline 13.5, scale 0.018; 13: baseline 11, scale 0.013 (upstream font metrics: 1000 units/em, 600-unit mono advance). The 17px slash may touch the upper edge, so inspect punctuation on the real panel.

**Existing Rev-C3 coverage is deliberately preserved:** 34px digits/time punctuation (15), 17px uppercase/date/market text (43), 13px numeric/weather characters (17). Lowercase and extra symbols from the earlier *comparison mock* have **not** yet been introduced into firmware; extending the font contract is a separate later change. These three fonts are currently diagnostic-only until user approval and the later Rev-C4 migration.

Validation to perform on the developer checkout:

```bash
python3 tools/generate_dashboard_fonts.py --check
```

In local ignored `config.h`, enable `#define EDP_PHASE8_FONT_TEST 1` and disable `EDP_PHASE8A1_PREVIEW`; Arduino compile/upload, then send `1`, `2`, `3`, `4` over 115200-baud Serial Monitor. Note whether glyphs/counters read clearly, the 17px uppercase `Q` tail and `/` survive, the 13px decimal and percent remain distinct, and text crosses x=396 cleanly. Capture all four sheets. **Do not mark the revision hardware-verified from source checks alone.**

## Rev-C3-JB2 — 14px candidate and side-by-side comparison (2026-10-10)

### Hardware feedback and scope

The user reports **34px and 17px JetBrains Mono appearance are excellent** after actual Rev-C3-JB1 panel testing; **13px is legible enough to evaluate but looks too small**. This is a visual acceptance of the two larger sizes, **not** approval of the full Dashboard layout or a completed regression matrix.

Keep the original 34, 17 and 13px pixels **byte-for-byte unchanged**. Generate an independent JetBrains Mono **Medium** 1-bit 9×14px candidate (9px monospaced advance, 14px cell height, no runtime scaling). The source is the same upstream official TTF blob recorded in Rev-C3-JB1; 4×4 outline coverage sampling, threshold 6/16, TrueType scale 0.01415 and baseline y=12.0 within the 14px cell. It is **not** derived by scaling 13px bitmaps. Its 17 glyphs match the 13px coverage: `0123456789.-/%+C `.

Files: `fonts_src/DashboardFont14.glyphs` is the editable 1-bit source, `DashboardFont14.h` is generated to the existing `BitmapFont` contract; the standard library-only `tools/generate_dashboard_fonts.py` now supports `--font 14` and includes 14px in `--check`.

### Five-page diagnostic (Serial 115200)

In ignored local `config.h`, enable `#define EDP_PHASE8_FONT_TEST 1` and ensure `EDP_PHASE8A1_PREVIEW` is not also enabled. Compile and upload `firmware/CrowPanelDashboard/CrowPanelDashboard.ino` from `phase8a1-static-prototype`; page 1 appears at boot. Send a single ASCII digit:

| Serial | Page | Notes |
| --- | --- | --- |
| `1` | 34px | Unchanged JetBrains Mono numbers (user visual PASS) |
| `2` | 17px | Unchanged JetBrains Mono capitals (user visual PASS) |
| `3` | 13px | Unchanged small-weather reference (user reports too small) |
| `4` | 14px | New 9×14px weather/number sample in the same positions as page 3 |
| `5` | 13 vs 14px | Identical strings side by side: 13px left / 14px right, with center divider at x=396 |

Page 5 shows identical digit, temperature, forecast, negative-temperature and percent patterns. Specifically compare decimal-point strength, `1/7`, percent, long-value fit, and legibility at usual viewing distance. Also inspect ghosting across successive page changes. Repeated same-page commands do not trigger physical refresh. The existing `maintenanceRefresh()` remains the **only** physical refresh path; no raw full refresh or extra background network services were added.

### Verification / status

- [x] Added 14px glyph matrices and generated 1-bit header, fixed width/advance 9px and line height 14px.
- [x] Static glyph/source/header and page bounds checks (code-level).
- [x] Updated five-page Serial selection and checked no 34/17/13 data were modified in this commit.
- [ ] Arduino ESP32-S3 compile/upload of JB2 revision.
- [ ] User hardware inspection of pages 4 and 5, then visual approval/rejection.
- [ ] Rev-C4 production/static Dashboard integration **only after user approval**.

The Python generator check is optional for user convenience; it is not required for Arduino IDE compile/upload. The 14px visual result and firmware compilation **cannot** be inferred from these static checks.
