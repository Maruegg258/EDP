# Phase 2 Graphics Verification

This document records hardware-verified behavior for the project-owned black/white graphics layer.

The SSD1683 refresh behavior remains owned by `CrowEPD579`. Phase 2 must not change the known-good Phase 1 refresh sequence unless a separate hardware investigation justifies it.

## Phase 2A-1 — Graphics layer extraction regression

**Status: Verified on hardware**

**Date:** 2026-09-29

**Firmware commit under test:** `294dfffee5044b0e06c35493b45ca1e9cc48bc41` — `Begin Phase 2A graphics layer extraction`

### Change under test

The following rendering responsibilities were moved out of `CrowPanelDashboard.ino` and into `GraphicsBW`:

- visible-coordinate to raw-framebuffer x mapping
- visible pixel writes
- filled rectangles
- 5x7 glyph rendering
- 5x7 text rendering
- text width calculation

`GraphicsBW` also gained `drawLine()` and `drawRect()`, but those primitives were verified separately in Phase 2A-2.

No `CrowEPD579` refresh code was changed by the extraction commit.

### Regression sequence

The existing Phase 1F sequence was intentionally retained:

1. Full-refresh baseline frame
2. One normal partial refresh
3. Maintenance refresh
4. Three normal partial refreshes

The test frame retained the centered `HELLO` text and one moving 32x32 black square.

### Hardware result

User-confirmed PASS on the physical CrowPanel 5.79-inch display.

Observed behavior matched the Phase 1F regression expectations:

- `HELLO` remained visually correct through the refresh sequence.
- The moving square behaved correctly, with only the current square remaining.
- No regression of the previously verified maintenance-refresh / follow-up-partial behavior was reported.

### Conclusion

The graphics extraction itself is hardware-verified and did not regress the known-good Phase 1 refresh behavior.

## Phase 2A-2 — Graphics primitives and seam mapping

**Status: Verified on hardware**

**Date:** 2026-09-29

**Firmware commit under test:** `cc7b2fa36a4fb83fd4095f17b6bfa3fdddfced66` — `Add Phase 2A primitives and seam hardware test`

### Test frame

The dedicated static full-frame test exercised:

- exact visible-screen border
- adjacent vertical lines at visible x=395 and x=396
- a long horizontal line crossing visible x=396
- an outlined rectangle centered across the controller seam
- horizontal and vertical line primitives away from the seam
- two diagonal lines crossing each other and the seam
- mirrored left/right reference geometry
- rectangles intentionally extending beyond the left and right visible edges
- explicit `setPixel()` writes immediately around the seam

The SSD1683 refresh sequence was not under test in this checkpoint; the frame used the already verified full-frame display path.

### Hardware result

User supplied a photograph of the physical panel and the rendered result matched the intended geometry.

Observed verification points:

- the outer border appeared complete
- the left/right intentionally clipped rectangles clipped at the visible edges without wraparound
- the long horizontal line crossed x=396 continuously
- the centered outlined rectangle crossed x=396 without an 8-pixel visual gap
- both diagonal lines crossed the controller boundary continuously
- the adjacent x=395/x=396 vertical lines appeared as the intended thicker central vertical reference
- normal pixel stair-stepping was visible on diagonal lines, consistent with 1-bit raster line drawing

No visible seam discontinuity or coordinate-mapping regression was observed.

### Conclusion

`setPixel()`, `drawLine()`, `drawRect()`, visible-edge clipping, and visible-to-raw seam mapping are hardware-verified for the Phase 2A test cases.

**Phase 2A is complete.**


## Phase 2B-1 — Generic BitmapFont abstraction

**Status: Verified on hardware**

**Date:** 2026-09-29

**Firmware commit under test:** `7f8dd4a882cdd103b168dcdf224a9e3d1d68e805` — `Add generic BitmapFont abstraction`

### Change under test

The text renderer was decoupled from the project-owned 5x7 font:

- added generic `BitmapGlyph` and `BitmapFont` data structures
- replaced `drawGlyph5x7()` with `drawGlyph(font, ...)`
- replaced `drawText5x7()` with `drawText(font, ...)`
- replaced `textWidth5x7()` with `textWidth(font, ...)`
- converted `Font5x7` into the first font definition using the generic interface
- retained only the existing H/E/L/O glyph set for this checkpoint

No SSD1683 refresh behavior was changed.

### Pre-hardware verification

Before flashing, the refactored code passed syntax/warning checks and the old/new HELLO framebuffer output was compared byte-for-byte with no difference.

### Hardware result

User-confirmed PASS on the physical CrowPanel 5.79-inch display:

- Arduino compile/upload completed normally
- `HELLO` rendered correctly
- no visible change in glyph shape, scale, or position compared with the pre-refactor 5x7 rendering

### Conclusion

The generic `BitmapFont` abstraction is hardware-verified for the existing 5x7 HELLO regression case.

Phase 2B can now proceed to Phase 2B-2: expand the base glyph set and exercise the generic font path with dashboard-style text and numeric content.


## Phase 2B-2 — Base character set and dashboard typography

**Status: Verified on hardware**

**Implementation date:** 2026-09-29

### Scope

The existing generic `BitmapFont` API is unchanged. This checkpoint only expands the project-owned `Font5x7` data and adds a dedicated typography test frame.

The font now covers:

- uppercase `A-Z`
- digits `0-9`
- space
- `:`
- `.`
- `-`
- `+`
- `%`
- `/`
- `(` and `)`

The previous H/E/L/O bitmaps are preserved.

### Test frame

The hardware test renders:

- `ABCDEFGHIJKLMNOPQRSTUVWXYZ`
- `0123456789  : . - + % / ( )`
- a large `12:34` clock sample
- `BTC 65234.50`
- `ETH 3921.75`
- `HYPE 48.26`
- `TEMP 28 C  WIFI -57`

Multiple scales are intentionally used to check readability and spacing.

No network data is used; all displayed values are static test content. No SSD1683 refresh behavior is changed by this checkpoint.


### Hardware result

User supplied a photograph of the physical CrowPanel 5.79-inch display and the Phase 2B-2 typography frame matched the intended content.

Verified points:

- A-Z rendered without missing glyphs or corruption
- 0-9 rendered correctly
- punctuation and spacing rendered correctly
- the large `12:34` sample was clearly readable
- BTC, ETH, HYPE, TEMP, and WIFI sample rows rendered without overlap or clipping
- multiple font scales rendered correctly through the same generic `BitmapFont` path

The compact 5x7 glyph designs remain intentionally utilitarian; some glyphs such as `Q`, `W`, and `%` have limited stylistic detail at this resolution, but this is a font-design limitation rather than a rendering defect.

### Conclusion

Phase 2B-2 is hardware-verified. The project now has a functional base uppercase/numeric/punctuation character set suitable for further dashboard layout work.

The next font checkpoint is Phase 2B-3: exercise the generic `BitmapFont` interface with multiple font definitions/sizes so later UI code is not tied to one 5x7 face.


## Phase 2B-3 — Multiple fonts and multi-byte glyph rows

**Status: Verified on hardware**

**Implementation date:** 2026-09-29

### Scope

A second project-owned font, `Font9x13`, is added without changing the generic `BitmapFont` interface or `GraphicsBW`.

The new font is numeric/status focused and includes:

- digits `0-9`
- space
- `:`
- `.`
- `-`
- `+`
- `%`
- `/`

The numeric glyphs are natively 9 pixels wide. With the existing row-major bitmap format this requires two bytes per glyph row, deliberately exercising the generic renderer beyond the one-byte rows used by `Font5x7`.

### Test frame

The dedicated hardware frame mixes both font definitions in the same framebuffer:

- `Font5x7` for labels and headings
- `Font9x13` for a native numeric/punctuation sample
- large `12:34` clock text
- right-aligned BTC, ETH, and HYPE numeric values
- `WIFI -57`
- `CHANGE +12.5%`

The test also exercises `textWidth()` through centered and right-aligned text.

No SSD1683 refresh behavior is changed by this checkpoint.


### Hardware result

User supplied a photograph of the physical CrowPanel 5.79-inch display and the mixed-font test frame matched the intended content.

Verified points:

- `Font5x7` labels and `Font9x13` numeric content rendered correctly in the same framebuffer.
- The native 9-pixel-wide numeric glyphs retained their rightmost column, confirming correct two-byte row handling.
- The large `12:34` sample rendered correctly and remained centered.
- BTC, ETH, and HYPE numeric values were right-aligned as intended.
- `WIFI -57` and `CHANGE +12.5%` rendered correctly.
- Punctuation `:`, `.`, `-`, `+`, `%`, and `/` rendered without corruption.

The visual style of `Font9x13` is intentionally seven-segment/digital and is only a renderer-validation font, not the final dashboard typography.

### Conclusion

Phase 2B-3 is hardware-verified. The generic `BitmapFont` renderer is now validated with multiple font definitions, multiple native glyph sizes, centered/right-aligned text, and glyph rows spanning more than one byte.

**Phase 2B is complete.**

The next graphics checkpoint is Phase 2C: generic bitmap/icon rendering.


## Phase 2C-1 — Generic 1-bit bitmap renderer

**Status: Verified on hardware**

**Implementation date:** 2026-09-29

### Scope

Phase 2C-1 adds a generic `Bitmap1bpp` data structure and `GraphicsBW::drawBitmap()`.

Bitmap format:

- row-major
- MSB-first
- each row padded to a whole byte
- set bits are foreground pixels
- clear bits are transparent
- foreground may be black or white

No icon-specific knowledge is added to `GraphicsBW`, and no SSD1683 refresh behavior is changed.

### Test asset

A dedicated asymmetric 33x33 verification bitmap is used. At 33 pixels wide it requires five bytes per row, exercising bitmap rows substantially wider than the previously tested fonts.

The marker includes a border, diagonals, center cross, and an asymmetric solid block near its upper-left corner so orientation and bit-order problems are visually obvious.

### Hardware test frame

The same bitmap is rendered:

- normally on a white background
- centered across visible x=396 to verify seam mapping
- partially beyond the right edge to verify clipping
- partially beyond the left edge to verify clipping
- in white on a black rectangle to verify foreground color and transparent zero bits

Phase 2C-1 remains pending until this frame is verified on the physical panel.


### Hardware result

User supplied a photograph of the physical CrowPanel 5.79-inch display and the Phase 2C-1 bitmap test frame matched the intended geometry.

Verified points:

- the normal 33x33 marker rendered with the expected border, diagonals, center cross, and asymmetric upper-left block
- the marker centered across visible x=396 rendered continuously with no controller-seam gap
- the right-clipped copy stopped at the visible right edge without wrapping
- the left-clipped copy stopped at the visible left edge without wrapping
- the white-on-black copy rendered the same marker shape in white while the bitmap's zero bits preserved the black background

### Conclusion

Phase 2C-1 is hardware-verified. The generic `Bitmap1bpp` path is now validated for multi-byte rows, visible-coordinate seam mapping, clipping on both horizontal edges, transparent zero bits, and black/white foreground rendering.

The next graphics checkpoint is Phase 2C-2: add project-owned Wi-Fi, weather, and crypto icon assets on top of the verified bitmap renderer.


## Phase 2C-2 — Project-owned dashboard icon assets

**Status: Verified on hardware**

**Implementation date:** 2026-09-29

### Scope

Phase 2C-2 adds the first project-owned icon asset set on top of the hardware-verified `Bitmap1bpp` renderer.

The icon data is stored directly in firmware and does not depend on external BMP files or third-party graphics libraries.

Initial assets:

- Wi-Fi disconnected
- Wi-Fi weak
- Wi-Fi medium
- Wi-Fi strong
- Weather sun
- Weather cloud
- Weather rain
- BTC
- ETH
- HYPE

Wi-Fi assets are 24x24. Weather and crypto assets are 32x32. The crypto symbols are project-drawn monochrome dashboard representations rather than imported artwork.

### Hardware test frame

The static icon-sheet test renders all ten icons with text labels in three sections:

1. Wi-Fi states
2. Weather conditions
3. Crypto symbols

No network service, weather provider, or market-data service is connected at this checkpoint. No SSD1683 refresh behavior or `GraphicsBW` bitmap logic is changed.

Phase 2C-2 remains pending until the icon sheet is visually verified on the physical panel.


### Hardware result

User supplied a photograph of the physical CrowPanel 5.79-inch display and the Phase 2C-2 icon sheet rendered as intended.

Verified points:

- Wi-Fi disconnected, weak, medium, and strong assets rendered without corruption and were visually distinguishable
- sun, cloud, and rain weather assets rendered completely; the rain strokes remained visible
- BTC, ETH, and HYPE crypto assets rendered without clipping, inversion, row misalignment, or missing data
- all ten icons rendered correctly through the already verified generic `Bitmap1bpp` path

The icon set is intentionally functional rather than final-polish artwork. In particular, the HYPE asset is a project-owned simplified hexagon/H representation and may be redesigned later during UI refinement.

### Conclusion

Phase 2C-2 is hardware-verified.

**Phase 2C is complete.**

The next checkpoint is Phase 2D: run a final Phase 2 integration regression and close out the graphics foundation before beginning the dashboard/widget framework.


## Phase 2D-1 — Static graphics integration frame

**Status: Verified on hardware**

**Implementation date:** 2026-09-29

### Scope

Phase 2D-1 does not add a new graphics primitive. It combines the already hardware-verified Phase 2 components into one dashboard-like static frame:

- `drawLine()`
- `drawRect()`
- `fillRect()`
- `Font5x7`
- `Font9x13`
- Wi-Fi bitmap
- weather bitmap
- BTC / ETH / HYPE bitmaps

All values are static test data. No Wi-Fi connection, time service, weather provider, or market-data service is active.

### Integration frame

The frame contains:

- sun icon and `28 C` weather summary
- large centered `12:34` clock
- Wi-Fi strong icon with `-57`
- BTC card with `65234.50`
- ETH card with `3921.75`
- HYPE card with `48.26`
- a black `STATIC TEST DATA` status bar with white text

The centered clock and the ETH card intentionally span the visible x=396 dual-controller boundary.

This checkpoint uses only the already verified full-frame refresh path. No SSD1683 refresh sequence is changed.

Phase 2D-1 remains pending until the complete integration frame is visually verified on hardware.


### Hardware result

User supplied a photograph of the physical CrowPanel 5.79-inch display and the Phase 2D-1 integration frame matched the intended dashboard-like layout.

Verified points:

- the centered `12:34` clock rendered continuously across the visible x=396 controller boundary
- the ETH card, which spans the controller boundary, rendered without a visible seam discontinuity
- BTC, ETH, and HYPE cards rendered with their icons, labels, numeric values, PERP labels, and card borders intact
- weather summary and Wi-Fi summary rendered correctly in the same framebuffer
- the outer frame and horizontal separator remained continuous
- the filled black status bar rendered correctly with white `STATIC TEST DATA` text
- `Font5x7`, `Font9x13`, bitmap icons, outlined geometry, and filled geometry coexisted correctly in one frame

### Conclusion

Phase 2D-1 is hardware-verified. The Phase 2 graphics components integrate correctly in a single dashboard-like static frame.

The next checkpoint is Phase 2D-2: re-run the proven partial + maintenance refresh sequence while changing mixed text and icon content.


## Phase 2D-2 — Mixed-content refresh regression

**Status: Hardware verification pending**

**Implementation date:** 2026-09-29

### Scope

Phase 2D-2 reuses the already hardware-verified Phase 1 refresh APIs without changing `CrowEPD579`.

The same dashboard-like integration layout is rebuilt from scratch for each state. Every state changes mixed content:

- clock text
- weather icon and label
- temperature
- Wi-Fi icon and RSSI text
- BTC / ETH / HYPE numeric values
- white text inside the filled black status bar

This intentionally requires both old black pixels to disappear and new black pixels to appear, including transparent bitmap areas and white-on-black text.

### Refresh sequence

1. Full baseline
   - `12:34`
   - sun / `28 C`
   - Wi-Fi strong / `-57`
2. Normal partial
   - `12:35`
   - cloud / `27 C`
   - Wi-Fi medium / `-64`
3. Maintenance refresh
   - `12:36`
   - rain / `26 C`
   - Wi-Fi weak / `-76`
4. First post-maintenance partial
   - `12:37`
5. Second post-maintenance partial
   - `12:38`
6. Third post-maintenance partial
   - `12:39`
   - rain / `25 C`
   - Wi-Fi disconnected / `-99`

Each frame also changes all three crypto price strings and the status-bar text.

The maintenance step remains the previously verified sequence:

`Fast clear -> physical white -> re-init -> previous RAM white -> current RAM new frame -> Partial refresh`

No new refresh mode is introduced.

### Pass criteria

Across all six visible states:

- previous clock digits must disappear cleanly
- old weather and Wi-Fi icon pixels must disappear cleanly
- new icons must remain sharp
- crypto price changes must not leave doubled/blurred digits
- the ETH card and centered clock must remain continuous across visible x=396
- changing white status text must not leave pale/white remnants in the black bar
- post-maintenance partials must remain as sharp as pre-maintenance content

Phase 2D-2 remains pending until the complete sequence is observed on the physical panel.
