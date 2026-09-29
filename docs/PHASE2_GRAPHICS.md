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

`GraphicsBW` also gained `drawLine()` and `drawRect()`, but those primitives are not yet considered hardware-verified by this checkpoint.

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

This does **not** yet verify every newly added graphics primitive. A dedicated geometry test is still required for `drawLine()` and `drawRect()`, especially across the visible x=396 controller seam.

## Next checkpoint — Phase 2A-2

Create a graphics-primitives test frame that exercises:

- horizontal, vertical, and diagonal lines
- outlined rectangles
- geometry near display edges
- geometry crossing visible x=396
- confirmation that the hidden raw framebuffer seam gap does not appear as an 8-pixel visual discontinuity

Phase 2A should remain **In progress** until this dedicated primitives/seam test passes on hardware.
