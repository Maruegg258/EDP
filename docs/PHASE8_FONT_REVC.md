# Phase 8A-1 Rev-C — Dashboard Font Reset

**Status (2026-10-10):** Rev-C1 typography specification and Rev-C2 new font assets are committed on `phase8a1-static-prototype`. Static source/header validation PASS. **Not yet compiled on ESP32-S3 or visually accepted on the device.**

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

## C3/C4 are intentionally not implemented yet

**Rev-C3:** Add a separate opt-in font-only diagnostic renderer, not a production dashboard change. It should show full 34px digit/symbol samples, 17px alphabet/date/labels, and 13px numeric/temperature/100%-precipitation samples. Split across screens instead of shrinking any text to force all content into one frame. Check consistent weight and true e-paper readability on the dual SSD1683 panel.

**Rev-C4:** After user approval of the test sheet, replace the experimental font references in `Phase8A1Preview.cpp`, recalculate text widths and only later address divider/layout changes and dynamic data binding.

## Scope and safety

All new font files are currently **unused by the active dashboard prototype**. Existing Rev-B `Phase8A1Digits.h`, `Phase8A1HeaderFont.h`, `Phase8A1SmallFont.h`, the renderer, `main` branch, E-paper driver, refresh sequence, and all services remain unchanged. No Wi-Fi secrets or external fonts were added. The new native pixel diagrams are authored for this project rather than redistributed from an external typeface.
