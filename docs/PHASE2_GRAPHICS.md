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
